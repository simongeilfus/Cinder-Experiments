#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class WireframeGeometryShaderApp : public App {
  public:
	WireframeGeometryShaderApp();
	void draw() override;
	
	gl::BatchRef mBatch;
};

WireframeGeometryShaderApp::WireframeGeometryShaderApp()
{
	// load the shader and create a batch
	gl::GlslProg::Format format;
	format.vertex( loadAsset( "shader.vert" ) )
	.fragment( loadAsset( "shader.frag" ) )
	.geometry( loadAsset( "shader.geom" ) );
	
	auto shader = gl::GlslProg::create( format );
	mBatch		= gl::Batch::create( geom::TorusKnot() >> geom::ColorFromAttrib( geom::NORMAL, []( vec3 n ) {
		return Colorf( n.x, n.y, n.z ); } ), shader );
	
	// enable depth testing
	gl::enableDepthWrite();
	gl::enableDepthRead();
}

void WireframeGeometryShaderApp::draw()
{
	gl::clear( Color::gray( 0.0f ) );
	
	// setup a basic camera
	gl::setMatrices( CameraPersp( getWindowWidth(), getWindowHeight(), 60, 1, 1000 ).calcFraming( Sphere( vec3( 0.0f ), 2.0f ) ) );
	gl::viewport( getWindowSize() );
	
	// render the batch with a small rotation
	gl::rotate( 0.15f * getElapsedSeconds(), vec3( 0.123, 0.456, 0.789 ) );
	mBatch->draw();
	
	getWindow()->setTitle( "Framerate: " + to_string( (int) getAverageFps() ) );
}

CINDER_APP( WireframeGeometryShaderApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )
