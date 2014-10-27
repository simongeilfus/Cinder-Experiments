#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"

#include "cinder/params/Params.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Shader.h"

#include "cinder/MayaCamUI.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "cinder/ObjLoader.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PBRTexturingApp : public AppNative {
  public:
	void setup() override;
	void update() override;
	void draw() override;
	void resize() override;
	
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	
	void setupParams();
	
	MayaCamUI				mMayaCam;
	
	gl::VboMeshRef			mMesh;
	
	gl::GlslProgRef			mShader;
	
	gl::Texture2dRef		mBaseColorMap;
	gl::Texture2dRef		mNormalMap;
	gl::Texture2dRef		mRoughnessMap;
	
	params::InterfaceGlRef	mParams;
	Perlin					mPerlin;
	
	float					mRoughness;
	float					mMetallic;
	float					mSpecular;
	float					mNormalDetails;
	Color					mBaseColor;
	
	bool					mAnimateLight;
	
	vector<vec3>			mLightPositions;
	vector<float>			mLightRadiuses;
	vector<Color>			mLightColors;
	float					mLightBRadius;
	float					mTime;
	
	float					mFocalLength, mSensorSize, mFStop;
	int						mFocalLengthPreset, mSensorSizePreset, mFStopPreset;
	int						mPrevFocalLengthPreset, mPrevSensorSizePreset, mPrevFStopPreset;
	float					mGamma;
};

static vector<string>	sFocalPresets			= { "Custom(mm)", "Super Wide(15mm)", "Wide Angle(25mm)", "Classic(36mm)", "Normal Lense(50mm)" };
static vector<float>	sFocalPresetsValues		= { -1.0f, 15.0f, 25.0f, 36.0f, 50.0f };
static vector<string>	sSensorPresets			= { "Custom(mm)", "8mm", "16mm", "22mm", "35mm", "70mm" };
static vector<float>	sSensorPresetsValues	= { -1.0f, 8.0f, 16.0f, 22.0f, 35.0f, 70.0f };
static vector<string>	sFStopPresets			= { "Custom", "f/1.4", "f/2", "f/2.8", "f/4", "f/5.6", "f/8" };
static vector<float>	sFStopPresetsValues		= { -1.0f, 1.4f, 2.0f, 2.8f, 4.0f, 5.6f, 8.0f };

void PBRTexturingApp::setup()
{
	// load the cave model
	mMesh	= gl::VboMesh::create( ObjLoader( loadAsset( "cave.obj" ) ) );
	
	// and textures
	mBaseColorMap = gl::Texture2d::create( loadImage( loadAsset( "Rock_baseColor.jpg" ) ),
										 gl::Texture2d::Format().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR ).mipmap().wrap( GL_REPEAT ).loadTopDown() );
	mNormalMap = gl::Texture2d::create( loadImage( loadAsset( "Rock_normal.jpg" ) ),
									   gl::Texture2d::Format().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR ).mipmap().wrap( GL_REPEAT ).loadTopDown() );
	mRoughnessMap = gl::Texture2d::create( loadImage( loadAsset( "Rock_roughness.jpg" ) ),
									   gl::Texture2d::Format().minFilter( GL_NEAREST_MIPMAP_NEAREST ).magFilter( GL_LINEAR ).mipmap().wrap( GL_REPEAT ).loadTopDown() );
	
	// prepare the Camera ui
	auto cam = CameraPersp();
	cam.setPerspective( 50.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	cam.setEyePoint( vec3( 12.710f, -1.723f, -12.086f ) );
	cam.setOrientation( quat(  -0.365f, -0.020f, -0.929f, 0.052f ) );
	mMayaCam.setCurrentCam( cam );
	
	// load and compile the shader and make sure it compile fine
	
	try {
		mShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	
	
	// set the initial parameters and setup the ui
	mRoughness			= 1.0f;
	mMetallic			= 0.0f;
	mSpecular			= 0.5f;
	mNormalDetails		= 1.0f;
	mLightRadiuses		= { 2.5f, 2.7f };
	mLightBRadius		= 2.0f;
	mLightPositions		= { vec3(5, 1, 0), vec3(-10, 3, 8) };
	mLightColors		= { Color(1.0f, 0.7f, 0.77f), Color( 0.5f, 0.6f, 1.0f ) };
	mBaseColor			= Color::gray( 0.5f );
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

void PBRTexturingApp::update()
{
	// animate the lights
	if( mAnimateLight ){
		Perlin p;
		vec3 target( cos( mTime * 0.5f ) * 8.0f, 4.0f + sin( mTime * 0.25f ) * 3.5f, sin( mTime * 0.5f ) * 8.0f );
		mLightPositions[0]	= mLightPositions[0] + ( target - mLightPositions[0] ) * 0.1f;

		mLightRadiuses[1]	= mLightBRadius * ( p.fBm( mTime * 0.5f ) + 1.0f ) - 0.5f;
		
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

void PBRTexturingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	// set the matrices
	
	gl::pushMatrices();
	gl::setMatrices( mMayaCam.getCamera() );
	
	// enable depth testing
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	{
		// set the shader
		gl::ScopedGlslProg shadingScp( mShader );
		
		// upload the shader uniforms
		mShader->uniform( "uLightPositions", &mLightPositions[0], mLightPositions.size() );
		vector<vec3> colors;
		colors.push_back( vec3( mLightColors[0].r, mLightColors[0].g, mLightColors[0].b ) );
		colors.push_back( vec3( mLightColors[1].r, mLightColors[1].g, mLightColors[1].b ) );
		mShader->uniform( "uLightColors", &colors[0], mLightPositions.size() );
		mShader->uniform( "uLightRadiuses", &mLightRadiuses[0], mLightPositions.size() );
		mShader->uniform( "uBaseColor", mBaseColor );
		mShader->uniform( "uSpecular", mSpecular );
		mShader->uniform( "uRoughness", pow( mRoughness, 4.0f ) );
		mShader->uniform( "uMetallic", mMetallic );
		mShader->uniform( "uDetails", mNormalDetails );
		mShader->uniform( "uExposure", lmap( mFocalLength / mFStop, 0.0f, 144.0f, 0.1f, 50.0f ) );
		mShader->uniform( "uGamma", mGamma );
		
		// bind textures and send their id's to the shader
		glActiveTexture( GL_TEXTURE0 );
		gl::ScopedTextureBind baseColorMapScp( mBaseColorMap );
		mShader->uniform( "uBaseColorMap", 0 );
		
		glActiveTexture( GL_TEXTURE1 );
		gl::ScopedTextureBind normalMapScp( mNormalMap );
		mShader->uniform( "uNormalMap", 1 );
		
		glActiveTexture( GL_TEXTURE2 );
		gl::ScopedTextureBind roughnessMapScp( mRoughnessMap );
		mShader->uniform( "uRoughnessMap", 2 );
		
		glActiveTexture( GL_TEXTURE0 );
		
		// render model
		gl::pushModelMatrix();
		
		// scale down
		gl::setModelMatrix( glm::scale( mat4(), vec3( 0.1f ) ) );
		gl::color( Color::white() );
		gl::draw( mMesh );
		gl::popModelMatrix();
	}
	
	gl::popMatrices();
	
	// render the ui
	mParams->draw();	
}

void PBRTexturingApp::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void PBRTexturingApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void PBRTexturingApp::resize()
{
	auto cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

void PBRTexturingApp::setupParams()
{
	mParams = params::InterfaceGl::create( "PBR", ivec2( 300, 400 ) );
	mParams->minimize();
	
	mParams->addText( "Material" );
	mParams->addParam( "Roughness", &mRoughness ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Metallic", &mMetallic ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Specular", &mSpecular ).min( 0.0f ).max( 1.0f ).step( 0.001f );
	mParams->addParam( "NormalDetails", &mNormalDetails ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Base Color", &mBaseColor );
	
	mParams->addSeparator();
	mParams->addText( "Lights" );
	mParams->addParam( "Light A Radius", &mLightRadiuses[0] ).min( 0.0f ).max( 20.0f ).step( 0.1f );
	mParams->addParam( "Light A Color", &mLightColors[0] );
	mParams->addParam( "Light B Radius", &mLightBRadius ).min( 0.0f ).max( 20.0f ).step( 0.1f );
	mParams->addParam( "Light B Color", &mLightColors[1] );
	mParams->addParam( "Animate Lights", &mAnimateLight );
	
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

CINDER_APP_NATIVE( PBRTexturingApp, RendererGl( RendererGl::Options().antiAliasing( RendererGl::AA_MSAA_16 ) ) )
