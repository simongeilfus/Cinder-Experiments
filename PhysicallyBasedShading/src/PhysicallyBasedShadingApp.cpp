#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Shader.h"

#include "cinder/MayaCamUI.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;


static vector<string>	sFocalPresets			= { "Custom(mm)", "Super Wide(15mm)", "Wide Angle(25mm)", "Classic(36mm)", "Normal Lense(50mm)" };
static vector<float>	sFocalPresetsValues		= { -1.0f, 15.0f, 25.0f, 36.0f, 50.0f };
static vector<string>	sSensorPresets			= { "Custom(mm)", "8mm", "16mm", "22mm", "35mm", "70mm" };
static vector<float>	sSensorPresetsValues	= { -1.0f, 8.0f, 16.0f, 22.0f, 35.0f, 70.0f };
static vector<string>	sFStopPresets			= { "Custom", "f/1.4", "f/2", "f/2.8", "f/4", "f/5.6", "f/8" };
static vector<float>	sFStopPresetsValues		= { -1.0f, 1.4f, 2.0f, 2.8f, 4.0f, 5.6f, 8.0f };

class PhysicallyBasedShadingApp : public AppNative {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void resize() override;
	
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	
	void setupParams();
	
	vec3					mLightPosition;
	MayaCamUI				mMayaCam;
	gl::VboMeshRef			mSphere;
	gl::GlslProgRef			mShader, mTextureShader;
	gl::Texture2dRef		mLightTexture;
	params::InterfaceGlRef	mParams;
	
	float					mRoughness;
	float					mMetallic;
	float					mSpecular;
	Color					mBaseColor;
	
	bool					mAnimateLight;
	float					mLightRadius;
	Color					mLightColor;
	float					mTime;
	
	float					mFocalLength, mSensorSize, mFStop;
	int						mFocalLengthPreset, mSensorSizePreset, mFStopPreset;
	int						mPrevFocalLengthPreset, mPrevSensorSizePreset, mPrevFStopPreset;
	float					mGamma;
};

void PhysicallyBasedShadingApp::setup()
{
	// build our test model
	mSphere = gl::VboMesh::create( geom::Sphere().subdivisions( 32 ) );
	
	// prepare the Camera ui
	auto cam = CameraPersp();
	cam.setPerspective( 50.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	mMayaCam.setCurrentCam( cam );
	
	// load and compile the shader and make sure they compile fine
	try {
		mShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	
	// load the the light texture and create a shader for it
	mTextureShader = gl::getStockShader( gl::ShaderDef().texture().color() );
	mLightTexture = gl::Texture2d::create( loadImage( loadAsset( "light.png" ) ) );
	
	// set the initial parameters and setup the ui
	mRoughness			= 1.0f;
	mMetallic			= 1.0f;
	mSpecular			= 1.0f;
	mLightRadius		= 4.0f;
	mLightColor			= Color( 1.0f, 0.025f, 0.9f );
	mBaseColor			= Color( 1.0f, 0.8f, 0.025f );
	mTime				= 0.0f;
	mAnimateLight		= true;
	mFStop				= 2.0f;
	mGamma				= 2.2f;
	mFocalLength		= 36.0f;
	mSensorSize			= 35.0f;
	mFocalLengthPreset	= mPrevFocalLengthPreset = 3;
	mSensorSizePreset	= mPrevSensorSizePreset = 4;
	mFStopPreset		= mPrevFStopPreset = 2;
	
	setupParams();
}

void PhysicallyBasedShadingApp::update()
{
	// animate the light
	if( mAnimateLight ){
		mLightPosition = vec3( cos( mTime * 0.5f ) * 8.0f, 8.0f + sin( mTime * 0.25f ) * 3.5f, sin( mTime * 0.5f ) * 8.0f );
		mTime += 0.025f;
	}
	
	// check for camera properties change
	CameraPersp cam = mMayaCam.getCamera();
	float sensorHeight = mSensorSize / getWindowAspectRatio();
	float fov = 180.0f / M_PI * 2.0f * math<float>::atan( 0.5f * sensorHeight / mFocalLength );
	
	if( mFocalLengthPreset != 0 && mFocalLengthPreset != mPrevFocalLengthPreset ){
		mFocalLength = sFocalPresetsValues[ mFocalLengthPreset ];
	}
	if( mSensorSizePreset != 0 && mSensorSizePreset != mPrevSensorSizePreset ){
		mSensorSize = sSensorPresetsValues[ mSensorSizePreset ];
	}
	if( mFStopPreset != 0 && mFStopPreset != mPrevFStopPreset ){
		mFStop = sFStopPresetsValues[ mFStopPreset ];
	}
	mPrevFocalLengthPreset	= mFocalLengthPreset;
	mPrevSensorSizePreset	= mSensorSizePreset;
	mPrevFStopPreset		= mFStopPreset;
	
	// update the fov if necessary
	if( cam.getFov() != fov ){
		cam.setFov( fov );
		mMayaCam.setCurrentCam( cam );
	}
}

void PhysicallyBasedShadingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	// set the shader and matrices
	gl::ScopedGlslProg shadingScp( mShader );
	gl::setMatrices( mMayaCam.getCamera() );
	
	// enable depth testing
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	// sends the base color, the specular opacity,
	// the light position, color and radius to the shader
	mShader->uniform( "uLightPosition", mLightPosition );
	mShader->uniform( "uLightColor", mLightColor );
	mShader->uniform( "uLightRadius", mLightRadius );
	mShader->uniform( "uBaseColor", mBaseColor );
	mShader->uniform( "uSpecular", mSpecular );
	
	// sends the tone-mapping uniforms
	// ( the 0.0f / 144.0f and 0.1f / 50.0f ranges are totally arbitrary )
	mShader->uniform( "uExposure", lmap( mFocalLength / mFStop, 0.0f, 144.0f, 0.1f, 50.0f ) );
	mShader->uniform( "uGamma", mGamma );
	
	// render a grid of sphere with different roughness/metallic values and colors
	int gridSize = 6;
	gl::pushModelMatrix();
	for( int x = -gridSize; x <= gridSize; x++ ){
		for( int z = -gridSize; z <= gridSize; z++ ){
			float roughness = lmap( (float) z, (float) -gridSize, (float) gridSize, 0.05f, 1.0f );
			float metallic	= lmap( (float) x, (float) -gridSize, (float) gridSize, 0.0f, 1.0f );
			
			mShader->uniform( "uRoughness", pow( roughness * mRoughness, 4.0f ) );
			mShader->uniform( "uMetallic", metallic * mMetallic );
			
			gl::setModelMatrix( glm::translate( vec3( x, 0, z ) * 2.25f ) );
			gl::draw( mSphere );
		}
	}
	gl::popModelMatrix();
	
	// render the light with a textured/billboarded quad
	gl::ScopedGlslProg texShaderScp( mTextureShader );
	gl::ScopedAdditiveBlend blendScp;
	gl::ScopedTextureBind textureScp( mLightTexture );
	gl::ScopedColor colorScp( mLightColor + Color::white() * 0.85f );
	
	vec3 right, up;
	mMayaCam.getCamera().getBillboardVectors( &right, &up );
	gl::drawBillboard( mLightPosition, vec2( mLightRadius * 0.75f ), 0.0f, right, up );
	
	// render the ui
	gl::disableDepthRead();
	gl::setMatricesWindow( getWindowSize() );
	mParams->draw();
}

void PhysicallyBasedShadingApp::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void PhysicallyBasedShadingApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void PhysicallyBasedShadingApp::resize()
{
	auto cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

void PhysicallyBasedShadingApp::setupParams()
{
	mParams = params::InterfaceGl::create( "PBR", ivec2( 300, 400 ) );
	mParams->minimize();
	
	mParams->addText( "Material" );
	mParams->addParam( "Roughness", &mRoughness ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Metallic", &mMetallic ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Specular", &mSpecular ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Base Color", &mBaseColor );
	
	mParams->addSeparator();
	mParams->addText( "Light" );
	mParams->addParam( "Animation", &mAnimateLight );
	mParams->addParam( "Radius", &mLightRadius ).min( 0.0f ).max( 20.0f ).step( 0.1f );
	mParams->addParam( "Color", &mLightColor );
	
	mParams->addSeparator();
	mParams->addText( "Camera" );
	mParams->addParam( "Focal Length", &mFocalLength ).min( 0.0f ).step( 1.0f ).updateFn( [this]() {
		if( mFocalLength != sFocalPresetsValues[mFocalLengthPreset] ){
			mFocalLengthPreset = 0;
		}
	} );
	mParams->addParam( "Lense", sFocalPresets, &mFocalLengthPreset );
	mParams->addParam( "Sensor Size", &mSensorSize ).min( 0.0f ).step( 1.0f ).updateFn( [this]() {
		if( mSensorSize != sSensorPresetsValues[mSensorSizePreset] ){
			mSensorSizePreset = 0;
		}
	} );
	mParams->addParam( "Film Gate", sSensorPresets, &mSensorSizePreset );
	mParams->addParam( "F-Stop", &mFStop ).min( 0.25f ).max( 8.0f ).step( 0.25f ).updateFn( [this]() {
		if( mFStop != sFStopPresetsValues[mFStopPreset] ){
			mFStopPreset = 0;
		}
	} );
	mParams->addParam( "F-Stop Preset", sFStopPresets, &mFStopPreset );
}

CINDER_APP_NATIVE( PhysicallyBasedShadingApp, RendererGl )
