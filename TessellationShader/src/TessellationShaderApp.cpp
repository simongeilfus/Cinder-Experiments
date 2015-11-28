#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

/*
 Tessellation Shader from Philip Rideout
 
 "Triangle Tessellation with OpenGL 4.0"
 http://prideout.net/blog/?p=48 */

class TessellationShaderApp : public App {
public:
	TessellationShaderApp();
	void draw() override;
	
	gl::BatchRef	mBatch;
	float			mInnerLevel, mOuterLevel;
};

TessellationShaderApp::TessellationShaderApp()
{
	// create a batch with our tesselation shader
	auto format = gl::GlslProg::Format()
	.vertex( loadAsset( "shader.vert" ) )
	.fragment( loadAsset( "shader.frag" ) )
	.geometry( loadAsset( "shader.geom" ) )
	.tessellationCtrl( loadAsset( "shader.cont" ) )
	.tessellationEval( loadAsset( "shader.eval" ) );
	auto shader	= gl::GlslProg::create( format );
	mBatch		= gl::Batch::create( geom::Icosahedron(), shader );
	
	mInnerLevel = 1.0f;
	mOuterLevel = 1.0f;
	
	// connect the keydown signal
	getWindow()->getSignalKeyDown().connect( [this](KeyEvent event) {
		switch ( event.getCode() ) {
			case KeyEvent::KEY_LEFT : mInnerLevel--; break;
			case KeyEvent::KEY_RIGHT : mInnerLevel++; break;
			case KeyEvent::KEY_DOWN : mOuterLevel--; break;
			case KeyEvent::KEY_UP : mOuterLevel++; break;
			case KeyEvent::KEY_1 : mBatch->replaceVboMesh( gl::VboMesh::create( geom::Cube() ) ); break;
			case KeyEvent::KEY_2 : mBatch->replaceVboMesh( gl::VboMesh::create( geom::Icosahedron() ) ); break;
			case KeyEvent::KEY_3 : mBatch->replaceVboMesh( gl::VboMesh::create( geom::Sphere() ) ); break;
			case KeyEvent::KEY_4 : mBatch->replaceVboMesh( gl::VboMesh::create( geom::Icosphere() ) ); break;
				
		}
		mInnerLevel = math<float>::max( mInnerLevel, 1.0f );
		mOuterLevel = math<float>::max( mOuterLevel, 1.0f );
	});
	
	gl::enableDepthWrite();
	gl::enableDepthRead();
	gl::disableBlending();
}
void TessellationShaderApp::draw()
{
	gl::clear( Color::gray( 0.35f ) );
	
	// setup basic camera
	auto cam = CameraPersp( getWindowWidth(), getWindowHeight(), 60, 1, 1000 ).calcFraming( Sphere( vec3( 0.0f ), 1.25f ) );
	gl::setMatrices( cam );
	gl::rotate( getElapsedSeconds() * 0.1f, vec3( 0.123, 0.456, 0.789 ) );
	gl::viewport( getWindowSize() );
	
	// update uniforms
	mBatch->getGlslProg()->uniform( "uTessLevelInner", mInnerLevel );
	mBatch->getGlslProg()->uniform( "uTessLevelOuter", mOuterLevel );
	
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

CINDER_APP( TessellationShaderApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )
