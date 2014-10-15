#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Shader.h"

#include "cinder/MayaCamUI.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PointLightShadowsApp : public AppNative {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	
	vec3				mPointLight;
	
	gl::FboCubeMapRef	mDepthMap;
	
	gl::GlslProgRef		mLightDepth;
	gl::GlslProgRef		mShading;
	gl::GlslProgRef		mColorOnly;
	
	MayaCamUI			mMayaCam;
	
	vector<pair<vec3,gl::VboMeshRef>> mScene;
};

void PointLightShadowsApp::setup()
{
	// Create a test scene with 4 walls, a ground,
	// a ceiling, a sphere in the center and 4 columns
	mScene = {
		// The room walls
		make_pair(
				  vec3( 0, -5, 0 ),
				  gl::VboMesh::create( geom::Plane().normal( vec3(0,1,0)).size( vec2( 20 ) ) )
				  ),
		make_pair(
				  vec3( 0, 15, 0 ),
				  gl::VboMesh::create( geom::Plane().normal( vec3(0,-1,0)).size( vec2( 20 ) ) )
				  ),
		make_pair(
				  vec3( 10, 5, 0 ),
				  gl::VboMesh::create( geom::Plane().normal( vec3(-1,0,0)).size( vec2( 20 ) ) )
				  ),
		make_pair(
				  vec3( -10, 5, 0 ),
				  gl::VboMesh::create( geom::Plane().normal( vec3(1,0,0)).size( vec2( 20 ) ) )
				  ),
		make_pair(
				  vec3( 0, 5, 10 ),
				  gl::VboMesh::create( geom::Plane().normal( vec3(0,0,-1)).size( vec2( 20 ) ) )
				  ),
		make_pair(
				  vec3( 0, 5, -10 ),
				  gl::VboMesh::create( geom::Plane().normal( vec3(0,0,1)).size( vec2( 20 ) ) )
				  ),
		// The sphere in the center
		make_pair(
				  vec3( 0, 5, 0 ),
				  gl::VboMesh::create( geom::Sphere().radius(3.0f).subdivisions(64) )
				  ),
		// Columns
		make_pair(
				  vec3( 5, -5, 5 ),
				  gl::VboMesh::create( geom::Cylinder().height( 20 ).radius( 0.5f ) )
				  ),
		make_pair(
				  vec3( -5, -5, 5 ),
				  gl::VboMesh::create( geom::Cylinder().height( 20 ).radius( 0.5f ) )
				  ),
		make_pair(
				  vec3( -5, -5, -5 ),
				  gl::VboMesh::create( geom::Cylinder().height( 20 ).radius( 0.5f ) )
				  ),
		make_pair(
				  vec3( 5, -5, -5 ),
				  gl::VboMesh::create( geom::Cylinder().height( 20 ).radius( 0.5f ) )
				  ),
	};
	
	// Create an fbo to store the depth of the scene from
	// the point of view of a point light
	gl::FboCubeMap::Format format;
	format.textureCubeMapFormat( gl::TextureCubeMap::Format().internalFormat( GL_RGBA32F ).magFilter( GL_LINEAR ).minFilter( GL_LINEAR ).wrap( GL_CLAMP_TO_EDGE ).mipmap(false) );
	mDepthMap = gl::FboCubeMap::create( 512, 512, format );
	
	// Load and compile the shaders.
	// Make sure they compile fine
	try {
		mLightDepth = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "LightDepth.vert" ) ).fragment( loadAsset( "LightDepth.frag" ) ) );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	try {
		mShading = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "Shading.vert" ) ).fragment( loadAsset( "Shading.frag" ) ) );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	
	mColorOnly = gl::getStockShader( gl::ShaderDef().color() );
	
	// Prepare the Camera ui
	auto cam = CameraPersp();
	cam.setPerspective( 50.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	mMayaCam.setCurrentCam( cam );
}

void PointLightShadowsApp::update()
{
	float time = getElapsedSeconds();
	vec3 lightTarget( cos( time * 0.5f ) * 8.0f, 4.0f + sin( time * 0.25f ) * 3.5f, sin( time * 0.5f ) * 8.0f );
	mPointLight += ( lightTarget - mPointLight ) * 0.1f;
}

void PointLightShadowsApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	// Render the scene in LightSpace to the cubemap fbo
	{
		gl::ScopedViewport viewPortScp( ivec2( 0 ), ivec2( 512 ) );
		gl::ScopedGlslProg shadingScp( mLightDepth );
		
		gl::enableDepthRead();
		gl::enableDepthWrite();
		
		for( uint8_t dir = 0; dir < 6; ++dir ) {
			// create and set the view and projection matrices
			// from the point of view of the light
			mat4 view, proj = glm::perspective( 90.0f, 1.0f, 1.0f, 1000.0f );
			switch ( GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir ) {
				case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
					view = glm::lookAt( mPointLight, mPointLight + glm::vec3( +1, +0, 0 ), glm::vec3( 0, -1, 0 ) );
					break;
				case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
					view = glm::lookAt( mPointLight, mPointLight + glm::vec3( -1, +0, 0 ), glm::vec3( 0, -1, 0 ) );
					break;
				case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
					view = glm::lookAt( mPointLight, mPointLight + glm::vec3( 0, +1, 0 ), glm::vec3( 0, 0, 1 ) );
					break;
				case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
					view = glm::lookAt( mPointLight, mPointLight + glm::vec3( 0, -1, 0 ), glm::vec3( 0, 0, -1 ) );
					break;
				case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
					view = glm::lookAt( mPointLight, mPointLight + glm::vec3( 0, 0, +1 ), glm::vec3( 0, -1, 0 ) );
					break;
				case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
					view = glm::lookAt( mPointLight, mPointLight + glm::vec3( 0, 0, -1 ), glm::vec3( 0, -1, 0 ) );
					break;
			}
			
			
			gl::setProjectionMatrix( proj );
			gl::setViewMatrix( view );
			
			// bind the cubemap fbo with the right texture selected
			mDepthMap->bindFramebufferFace( GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir );
			
			gl::clear();
			
			// render the scene
			for( auto object : mScene ){
				gl::pushModelMatrix();
				gl::setModelMatrix( glm::translate( object.first ) );
				gl::draw( object.second );
				gl::popModelMatrix();
			}
		}
		
		gl::FboCubeMap::unbindFramebuffer();
	}
	
	// Render the scene in Camera space
	{
		gl::setMatrices( mMayaCam.getCamera() );
		gl::ScopedViewport viewPortScp( ivec2( 0 ), getWindowSize() );
		gl::ScopedGlslProg shadingScp( mShading );
		gl::ScopedFaceCulling cullingScp( true, GL_BACK );
		gl::ScopedTextureBind depthMapScp( mDepthMap->getTexture( GL_COLOR_ATTACHMENT0 ) );
		
		gl::enableDepthRead();
		gl::enableDepthWrite();
		
		
		mShading->uniform( "uDepthMap", 0 );
		mShading->uniform( "uLightPosition", mPointLight );
		
		
		for( auto object : mScene ){
			gl::pushModelMatrix();
			gl::setModelMatrix( glm::translate( object.first ) );
			gl::draw( object.second );
			gl::popModelMatrix();
		}
		
		
		gl::ScopedGlslProg colorOnlyScp( mColorOnly );
		gl::pushModelMatrix();
		gl::setModelMatrix( glm::translate( mPointLight ) );
		gl::drawCoordinateFrame();
		gl::popModelMatrix();
	}
}

void PointLightShadowsApp::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void PointLightShadowsApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_NATIVE( PointLightShadowsApp, RendererGl )
