#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ViewportArrayApp : public App {
public:
	void setup() override;
	void resize() override;
	void update() override;
	void draw() override;
	
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseWheel( MouseEvent event ) override;
	
	void updateViewports();
	
	gl::FboRef		mFbo;
	gl::BatchRef		mTeapot;
	
	array<CameraPersp,4>	mCameras;
	CameraUi		mCameraUi;
	vector<Rectf>		mViewports;
};

void ViewportArrayApp::setup()
{
	// create the teapot batch with our multicast geometry shader
	auto shForm = gl::GlslProg::Format().vertex( loadAsset( "shader.vert" ) ).geometry( loadAsset( "shader.geom" ) ).fragment( loadAsset( "shader.frag" ) );
	auto shader = gl::GlslProg::create( shForm );
	auto geom = geom::Teapot().subdivisions(10) >> geom::ColorFromAttrib(geom::Attrib::NORMAL, (const std::function<Colorf(vec3)>&) [](vec3 v){ return Colorf(v.x, v.y, v.z); });
	mTeapot = gl::Batch::create( geom, shader );
	
	// create 4 cameras randomly spread around the teapot
	randSeed( 12345 );
	for( auto &camera : mCameras ) {
		camera = CameraPersp( getWindowWidth(), getWindowHeight(), 50, 0.1f, 100.0f );
		camera.lookAt( randVec3() * randFloat( 1.5f, 3.0f ), vec3( 0.0f, 0.35f, 0.0f ) );
	}
}
void ViewportArrayApp::resize()
{
	// update each aspect/resolution dependant objects
	mFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), gl::Fbo::Format().samples( 16 ) );
	mCameraUi.setWindowSize( getWindowSize() / 2 );
	for( auto &camera : mCameras ) {
		camera.setAspectRatio( getWindowAspectRatio() );
	}
	// and recreate/upload each viewports
	updateViewports();
}
void ViewportArrayApp::update()
{
	// bind the fbo and enable depth testing
	gl::ScopedFramebuffer scopedFbo( mFbo );
	gl::ScopedDepth scopedDepthTest( true );
	
	// clear our fbo
	gl::clear( Color( 0, 0, 0 ) );
	
	// create an array with the different cameras matrices
	vector<mat4> matrices;
	for( auto cam : mCameras ) {
		matrices.push_back( cam.getProjectionMatrix() * cam.getViewMatrix() );
	}
	// and send it to the shader
	mTeapot->getGlslProg()->uniform( "uMatrices", &matrices[0], mCameras.size() );
	
	// render the teapot once (the geometry shader takes care of the rest)
	mTeapot->draw();
}

void ViewportArrayApp::draw()
{
	// clear the screen and set matrices
	gl::clear( Color( 0, 0, 0 ) );
	gl::setMatricesWindow( getWindowSize() );
	
	// render our fbo texture
	gl::draw( mFbo->getColorTexture() );
	
	// and viewport bounds
	for( auto viewport : mViewports ) {
		gl::drawStrokedRect( viewport );
	}
}

void ViewportArrayApp::updateViewports()
{
	// update the viewport array
	mViewports = {
		Rectf( vec2( 0.0f, 0.0f ), vec2( getWindowCenter().x, getWindowCenter().y ) ),
		Rectf( vec2( getWindowCenter().x, 0.0f ), vec2( getWindowSize().x, getWindowCenter().y ) ),
		Rectf( vec2( 0.0f, getWindowCenter().y ), vec2( getWindowCenter().x, getWindowSize().y ) ),
		Rectf( vec2( getWindowCenter().x, getWindowCenter().y ), vec2( getWindowSize().x, getWindowSize().y ) )
	};
	
	// transform it to a float array that opengl can understand
	// the first viewport will be the main one used in the draw function
	vector<float> viewportsf;
	viewportsf.push_back( 0.0f );
	viewportsf.push_back( 0.0f );
	viewportsf.push_back( getWindowWidth() );
	viewportsf.push_back( getWindowHeight() );
	for( auto viewport : mViewports ) {
		viewportsf.push_back( viewport.getX1() );
		viewportsf.push_back( getWindowHeight() - viewport.getY2() );
		viewportsf.push_back( viewport.getWidth() );
		viewportsf.push_back( viewport.getHeight() );
	}
	
	// upload the viewport array
	glViewportArrayv( 0, 5, &viewportsf[0] );
}


int getViewportAt( const vector<Rectf> &viewports, const ci::vec2 &position )
{
	int viewport = -1;
	for( size_t i = 0; i < viewports.size(); ++i ){
		if( viewports[i].contains( position ) ) {
			viewport = i;
			break;
		}
	}
	return viewport;
}

static int currentViewport;
void ViewportArrayApp::mouseDown( MouseEvent event )
{
	int viewport = currentViewport = getViewportAt( mViewports, event.getPos() );
	if( viewport != -1 ) mCameraUi.setCamera( &mCameras[viewport] );
	mCameraUi.mouseDown( event );
}

void ViewportArrayApp::mouseDrag( MouseEvent event )
{
	if( currentViewport != -1 ) mCameraUi.setCamera( &mCameras[currentViewport] );
	mCameraUi.mouseDrag( event );
}
void ViewportArrayApp::mouseUp( MouseEvent event )
{
	if( currentViewport != -1 ) mCameraUi.setCamera( &mCameras[currentViewport] );
	mCameraUi.mouseUp( event );
}
void ViewportArrayApp::mouseWheel( MouseEvent event )
{
	int viewport = getViewportAt( mViewports, event.getPos() );
	if( viewport != -1 ) mCameraUi.setCamera( &mCameras[viewport] );
	mCameraUi.mouseWheel( event );
}


CINDER_APP( ViewportArrayApp, RendererGl )
