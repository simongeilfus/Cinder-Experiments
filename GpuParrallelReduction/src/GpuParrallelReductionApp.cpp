#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Query.h"
#include "cinder/CameraUi.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GpuParrallelReductionApp : public App {
  public:
	GpuParrallelReductionApp();
	void update() override;
	void draw() override;
	void resize() override;
	
	vec4 findMindMax();
	
	CameraPersp	mCamera;
	CameraUi	mCameraUi;
	
	gl::FboRef	mFbo;
	
	double		mReductionTime;
	double		mReadBackTime;
};

GpuParrallelReductionApp::GpuParrallelReductionApp()
{
	// create a Camera and a Camera ui
	mCamera		= CameraPersp( getWindowWidth(), getWindowHeight(), 50.0f, 1.0f, 1000.0f ).calcFraming( Sphere( vec3( 0.0f ), 2.0f ) );
	mCameraUi	= CameraUi( &mCamera, getWindow(), -1 );
	
	// setup a framebuffer
	mFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight() );
}
void GpuParrallelReductionApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
	mFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight() );
}
void GpuParrallelReductionApp::update()
{
	{
		gl::ScopedFramebuffer scopedFbo( mFbo );
		gl::ScopedViewport scopedViewport( vec2(0), mFbo->getSize() );
		gl::ScopedDepth scopedDepth( true );
		gl::ScopedBlend scopedBlend( false );
		gl::setMatrices( mCamera );
	
		gl::clear( ColorA::gray( 0.0f ) );
		gl::drawColorCube( vec3(0), vec3(1) );
		gl::drawCube( vec3(0,0,-0.5), vec3(0.1) );
	}
}

void GpuParrallelReductionApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	gl::setMatricesWindow( getWindowSize() );
	gl::draw( mFbo->getColorTexture() );
	
	auto max = findMindMax();
	gl::drawStringCentered( "Current Max value: " + toString( max ), getWindowCenter() - vec2( 0, 10 ) );
	gl::drawStringCentered( "Reduction time " + to_string( mReductionTime ) + " ms", getWindowCenter() + vec2( 0, 12 ) );
	gl::drawStringCentered( "Read back time " + to_string( mReadBackTime ) + " ms", getWindowCenter() + vec2( 0, 25 ) );
}
vec4 GpuParrallelReductionApp::findMindMax()
{
	// init the programs, fbo and texture used for the parrallel reduction
	static gl::FboRef sReductionFbo, sReadFbo;
	static gl::Texture2dRef sReductionTexture0;
	static gl::GlslProgRef sReductionProg, sCopyProg;
	auto startingSize		= mFbo->getSize() / 2;
	if( !sReductionFbo || sReductionFbo->getSize() != startingSize ) {
		sReductionTexture0	= gl::Texture2d::create( startingSize.x, startingSize.y, gl::Texture2d::Format().minFilter( GL_NEAREST_MIPMAP_NEAREST ).magFilter( GL_NEAREST ).mipmap().immutableStorage() );
		sReductionFbo		= gl::Fbo::create( startingSize.x, startingSize.y, gl::Fbo::Format()
											  .attachment( GL_COLOR_ATTACHMENT0, sReductionTexture0 )
											  .disableDepth() );
		sReductionProg		= gl::GlslProg::create( loadAsset( "minMax.vert" ), loadAsset( "minMax.frag" ) );
	}
	// start reduction profiling
	static auto sTimer0 = gl::QueryTimeSwapped::create();
	sTimer0->begin();
	
	// attach the main level of the texture
	gl::ScopedFramebuffer scopedFbo( sReductionFbo );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sReductionTexture0->getId(), 0 );
	
	// start by blitting the main fbo into the reduction one
	mFbo->blitTo( sReductionFbo, mFbo->getBounds(), sReductionFbo->getBounds(), GL_NEAREST );
	
	
	// bind the reduction program and texture and disable blending
	gl::ScopedMatrices scopedMatrices;
	gl::ScopedGlslProg scopedGlsl( sReductionProg );
	gl::ScopedBlend scopedBlend( false );
	gl::ScopedTextureBind scopedTexBind0( sReductionTexture0, 0 );
	sReductionProg->uniform( "uTex0", 0 );
	
	// iterate trough each mipmap level
	int numMipMaps = gl::Texture2d::requiredMipLevels( startingSize.x, startingSize.y, 0 );
	for( int level = 1;	level < numMipMaps; ++level ) {
		// attach the current mipmap level to the framebuffer and limit texture sampling to the previous level
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, level - 1 );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level - 1 );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sReductionTexture0->getId(), level );
		
		// get the current mipmap size and update uniforms
		vec2 size = gl::Texture2d::calcMipLevelSize( level, sReductionFbo->getWidth(), sReductionFbo->getHeight() );
		sReductionProg->uniform( "uInvSize", vec2( 1.0f ) / vec2( size ) );
		
		// render a fullscreen quad
		gl::ScopedViewport scopedViewport( ivec2( 0 ), size );
		gl::setMatricesWindow( size.x, size.y );
		gl::drawSolidRect( Rectf( vec2( 0.0f ), vec2( size ) ) );
	}
	// stop reduction profiling
	sTimer0->end();
	mReductionTime = sTimer0->getElapsedMilliseconds();
	
	// start readback profiling
	static auto sTimer1 = gl::QueryTimeSwapped::create();
	sTimer1->begin();
	
	// read back to the cpu and find the max value
	vec4 max( 0.0f );
	ivec2 readSize = gl::Texture2d::calcMipLevelSize( numMipMaps - 1, startingSize.x, startingSize.y );
	Surface8u surface( readSize.x, readSize.y, true );
	glGetTexImage( sReductionTexture0->getTarget(), numMipMaps - 1, GL_RGBA, GL_UNSIGNED_BYTE, surface.getData() );
	auto it = surface.getIter();
	while( it.line() ) { while( it.pixel() ) {
		max = glm::max( max, vec4( it.r(), it.g(), it.b(), it.a() ) );
	} }
	
	// stop readback profiling
	sTimer1->end();
	mReadBackTime = sTimer1->getElapsedMilliseconds();
	
	return max;
}

CINDER_APP( GpuParrallelReductionApp, RendererGl )
