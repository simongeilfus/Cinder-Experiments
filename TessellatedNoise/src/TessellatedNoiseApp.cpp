#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"

using namespace ci;
using namespace ci::app;
using namespace std;

/*
 Tessellation Shader from Philip Rideout
 
 "Triangle Tessellation with OpenGL 4.0"
 http://prideout.net/blog/?p=48 */

class TessellatedNoiseApp : public App {
public:
	TessellatedNoiseApp();
	void draw() override;
	
	CameraPersp				mCamera;
	CameraUi				mCameraUi;
	gl::BatchRef			mBatch;
	float					mInnerLevel, mOuterLevel;
	gl::TextureCubeMapRef	mIrradianceMap, mRadianceMap;
};

TessellatedNoiseApp::TessellatedNoiseApp()
{
	// add the common texture folder
	addAssetDirectory( fs::path( __FILE__ ).parent_path().parent_path().parent_path() / "common/textures" );
	
	// create a batch with our tesselation shader
	auto format = gl::GlslProg::Format()
	.vertex( loadAsset( "shader.vert" ) )
	.fragment( loadAsset( "shader.frag" ) )
	.geometry( loadAsset( "shader.geom" ) )
	.tessellationCtrl( loadAsset( "shader.cont" ) )
	.tessellationEval( loadAsset( "shader.eval" ) );
	auto shader	= gl::GlslProg::create( format );
	mBatch = gl::Batch::create( geom::Icosphere().subdivisions( 3 ), shader );
	
	// load the prefiltered IBL Cubemaps
	auto cubeMapFormat	= gl::TextureCubeMap::Format().mipmap().internalFormat( GL_RGB16F ).minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR );
	mIrradianceMap		= gl::TextureCubeMap::createFromDds( loadAsset( "WellsIrradiance.dds" ), cubeMapFormat );
	mRadianceMap		= gl::TextureCubeMap::createFromDds( loadAsset( "WellsRadiance.dds" ), cubeMapFormat );
	
	mInnerLevel = 8.0f;
	mOuterLevel = 6.0f;
	
	// connect the keydown signal
	getWindow()->getSignalKeyDown().connect( [this](KeyEvent event) {
		switch ( event.getCode() ) {
			case KeyEvent::KEY_LEFT : mInnerLevel--; break;
			case KeyEvent::KEY_RIGHT : mInnerLevel++; break;
			case KeyEvent::KEY_DOWN : mOuterLevel--; break;
			case KeyEvent::KEY_UP : mOuterLevel++; break;
				
		}
		mInnerLevel = math<float>::max( mInnerLevel, 1.0f );
		mOuterLevel = math<float>::max( mOuterLevel, 1.0f );
	});

	
	mCamera = CameraPersp( getWindowWidth(), getWindowHeight(), 50, 0.1f, 1000 ).calcFraming( Sphere( vec3( 0.0f ), 1.125f ) );
	mCameraUi = CameraUi( &mCamera, getWindow() );
	
	gl::enableDepthWrite();
	gl::enableDepthRead();
	gl::disableBlending();
}
void TessellatedNoiseApp::draw()
{
	gl::clear( Color::black() );
	
	// setup matrices
	gl::setMatrices( mCamera );
	gl::viewport( getWindowSize() );
	
	// update uniforms and bind the cubemap textures
	gl::ScopedTextureBind scopedTexBind0( mRadianceMap, 0 );
	gl::ScopedTextureBind scopedTexBind1( mIrradianceMap, 1 );
	auto shader = mBatch->getGlslProg();
	shader->uniform( "uRadianceMap", 0 );
	shader->uniform( "uIrradianceMap", 1 );
	shader->uniform( "uRadianceMapSize", (float) mRadianceMap->getWidth() );
	shader->uniform( "uIrradianceMapSize", (float) mIrradianceMap->getWidth() );
	shader->uniform( "uTessLevelInner", mInnerLevel );
	shader->uniform( "uTessLevelOuter", mOuterLevel );
	shader->uniform( "uTime", 10000.0f + (float) getElapsedSeconds() );

	
	// bypass gl::Batch::draw method so we can use GL_PATCHES
	gl::ScopedVao scopedVao( mBatch->getVao().get() );
	gl::ScopedGlslProg scopedShader( mBatch->getGlslProg() );
	
	gl::context()->setDefaultShaderVars();
	
	if( mBatch->getVboMesh()->getNumIndices() )
		glDrawElements( GL_PATCHES, mBatch->getVboMesh()->getNumIndices(), mBatch->getVboMesh()->getIndexDataType(), (GLvoid*)( 0 ) );
	else
		glDrawArrays( GL_PATCHES, 0, mBatch->getVboMesh()->getNumIndices() );
	
	getWindow()->setTitle( "Framerate: " + to_string( (int) getAverageFps() ) );
}

CINDER_APP( TessellatedNoiseApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )
