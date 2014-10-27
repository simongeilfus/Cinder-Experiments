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
	
	gl::TextureRef			mCopyrightMap;
	gl::Texture2dRef		mBaseColorMap;
	gl::Texture2dRef		mNormalMap;
	gl::Texture2dRef		mRoughnessMap;
	gl::Texture2dRef		mMetallicMap;
	gl::Texture2dRef		mEmissiveMap;
	
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
	// load the leprechaun model
	TriMesh mesh = TriMesh( TriMesh::Format().normals().texCoords().positions() );
	mesh.read( loadAsset( "leprechaun.msh" ) );
	mMesh	= gl::VboMesh::create( mesh );
	
	// and textures
	mBaseColorMap = gl::Texture2d::create( loadImage( loadAsset( "leprechaun_baseColor.jpg" ) ),
										 gl::Texture2d::Format().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR ).mipmap().wrap( GL_REPEAT ).loadTopDown() );
	mNormalMap = gl::Texture2d::create( loadImage( loadAsset( "leprechaun_normal.jpg" ) ),
									   gl::Texture2d::Format().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR ).mipmap().wrap( GL_REPEAT ).loadTopDown() );
	mRoughnessMap = gl::Texture2d::create( loadImage( loadAsset( "leprechaun_roughness.jpg" ) ),
									   gl::Texture2d::Format().minFilter( GL_NEAREST_MIPMAP_NEAREST ).magFilter( GL_LINEAR ).mipmap().wrap( GL_REPEAT ).loadTopDown() );
	mMetallicMap = gl::Texture2d::create( loadImage( loadAsset( "leprechaun_metallic.jpg" ) ),
										 gl::Texture2d::Format().minFilter( GL_NEAREST_MIPMAP_NEAREST ).magFilter( GL_LINEAR ).mipmap().wrap( GL_REPEAT ).loadTopDown() );
	mEmissiveMap = gl::Texture2d::create( loadImage( loadAsset( "leprechaun_emissive.png" ) ),
										 gl::Texture2d::Format().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR ).mipmap().loadTopDown() );
	
	// load our copyright message
	mCopyrightMap  = gl::Texture::create( loadImage( loadAsset( "copyright.png" ) ) );
	
	// prepare the Camera ui
	auto cam = CameraPersp();
	cam.setPerspective( 50.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	cam.setEyePoint( vec3( -8.223f, 12.029f, 40.450f ) );
	cam.setOrientation( quat( -0.993f, 0.049f, 0.108f, 0.006f ) );
	mMayaCam.setCurrentCam( cam );
	
	// load and compile the shader and make sure it compile fine
	
	try {
		mShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	
	
	// set the initial parameters and setup the ui
	mRoughness			= 1.0f;
	mMetallic			= 1.0f;
	mSpecular			= 1.0f;
	mNormalDetails		= 1.0f;
	mLightRadiuses		= { 8.0f, 5.0f };
	mLightPositions		= { vec3(0), vec3(0) };
	mLightColors		= { Color(0.2f, 0.6f, 1.0f), Color(0.9f, 0.6f, 0.3f) };
	mBaseColor			= Color::white();
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
		mLightPositions[0] = vec3( cos( M_PI + mTime * 0.1f ) * 12.0f, 8.0f + sin( mTime * 0.05f ) * 3.5f, sin( M_PI + mTime * 0.1f ) * 12.0f );
		mLightPositions[1] = vec3(3.125f, 7.5f, 3.125f) + mPerlin.dfBm( vec3( 0.0f, 0.0f, mTime * 0.5f ) ) * 0.5f;
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
		
		glActiveTexture( GL_TEXTURE3 );
		gl::ScopedTextureBind metallicMapScp( mMetallicMap );
		mShader->uniform( "uMetallicMap", 3 );
		
		glActiveTexture( GL_TEXTURE4 );
		gl::ScopedTextureBind aoMapScp( mEmissiveMap );
		mShader->uniform( "uEmissiveMap", 4 );
		
		glActiveTexture( GL_TEXTURE0 );
		
		// render model
		gl::pushModelMatrix();
		
		// scale down
		gl::setModelMatrix( glm::scale( mat4(), vec3( 0.25f ) ) );
		gl::color( Color::white() );
		gl::draw( mMesh );
		gl::popModelMatrix();
		
		// render a sphere so we got a background
		// this is needed by the scattering code
		gl::color( Color::black() );
		gl::drawSphere( vec3(0), 500.0f, 16 );
	}
	
	gl::popMatrices();
	
	// render the ui
	mParams->draw();
	
	// render the copyright message
	{
		gl::ScopedGlslProg glslProgScope( gl::getStockShader( gl::ShaderDef().texture() ) );
		
		Area centered = Area::proportionalFit( mCopyrightMap->getBounds(), getWindowBounds(), true, false );
		centered.offset( ivec2( 0, ( getWindowHeight() - centered.y2 ) - 20 ) );
		
		gl::color( Color::white() );
		gl::enableAlphaBlending();
		gl::draw( mCopyrightMap, centered );
		gl::disableAlphaBlending();
	}
	
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
	mParams->addParam( "Ambient Radius", &mLightRadiuses[0] ).min( 0.0f ).max( 20.0f ).step( 0.1f );
	mParams->addParam( "Ambient Color", &mLightColors[0] );
	mParams->addParam( "Lantern Radius", &mLightRadiuses[1] ).min( 0.0f ).max( 20.0f ).step( 0.1f );
	mParams->addParam( "Lantern Color", &mLightColors[1] );
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
