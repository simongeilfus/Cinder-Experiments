#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"

#include "cinder/MayaCamUI.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PhysicallyBasedShadingApp : public AppNative {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	
	vec3			mPointLight;
	MayaCamUI		mMayaCam;
	gl::VboMeshRef	mSphere;
	gl::GlslProgRef	mShader;
};

void PhysicallyBasedShadingApp::setup()
{
	// Build our test model
	mSphere = gl::VboMesh::create( geom::Sphere().subdivisions( 32 ) );
	
	// Prepare the Camera ui
	auto cam = CameraPersp();
	cam.setPerspective( 50.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	mMayaCam.setCurrentCam( cam );
	
	// Load and compile the shader and make sure they compile fine
	try {
		mShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
}

void PhysicallyBasedShadingApp::update()
{
	// animate the light
	float time = getElapsedSeconds();
	mPointLight = vec3( cos( time * 0.5f ) * 8.0f, 8.0f + sin( time * 0.25f ) * 3.5f, sin( time * 0.5f ) * 8.0f );
}

void PhysicallyBasedShadingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	// Set the shader and matrices
	gl::ScopedGlslProg shadingScp( mShader );
	gl::setMatrices( mMayaCam.getCamera() );
	
	// enable depth testing
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	// upload the position of the light to the shader
	mShader->uniform( "uLightPosition", mPointLight );
	
	// renders a grid of sphere with different roughness/metallic values and colors
	
	int gridSize = 6;
	
	gl::pushModelMatrix();
	for( int x = -gridSize; x < gridSize; x++ ){
		for( int z = -gridSize; z < gridSize; z++ ){
			float roughness = lmap( (float) z, (float) -gridSize, (float) gridSize, 0.01f, 1.0f );
			float metallic	= lmap( (float) x, (float) -gridSize, (float) gridSize, 0.0f, 1.0f );
			
			mShader->uniform( "uRoughness", roughness );
			mShader->uniform( "uMetallic", metallic );
			
			Color c( ColorModel::CM_HSV, vec3( 1.0f - roughness * metallic * 0.25f, 1.0f, 1.0f ) );
			mShader->uniform( "uBaseColor", vec3( c.r, c.g, c.b ) );
			
			gl::setModelMatrix( glm::translate( vec3( x, 0, z ) * 2.25f ) );
			gl::draw( mSphere );
		}
	}
	gl::popModelMatrix();
}

void PhysicallyBasedShadingApp::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void PhysicallyBasedShadingApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

CINDER_APP_NATIVE( PhysicallyBasedShadingApp, RendererGl )
