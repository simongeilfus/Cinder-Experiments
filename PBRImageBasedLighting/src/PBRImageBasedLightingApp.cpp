#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PBRImageBasedLightingApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void resize() override;
	
	CameraPersp				mCamera;
	CameraUi				mCameraUi;
	gl::BatchRef			mModelBatch, mSkyBoxBatch;
	gl::TextureCubeMapRef	mIrradianceMap, mRadianceMap;
	
	int						mGridSize;
	bool					mShowUi, mRotateModel;
	float					mRoughness, mMetallic, mSpecular;
	Color					mBaseColor;
	float					mGamma, mExposure, mTime;
};

void PBRImageBasedLightingApp::setup()
{
	// add the common texture folder
	addAssetDirectory( fs::path( __FILE__ ).parent_path().parent_path().parent_path() / "common/textures" );
	
	// create a Camera and a Camera ui
	mCamera		= CameraPersp( getWindowWidth(), getWindowHeight(), 50.0f, 1.0f, 1000.0f ).calcFraming( Sphere( vec3( 0.0f ), 12.0f ) );
	mCameraUi	= CameraUi( &mCamera, getWindow(), -1 );
	
	// prepare ou rendering objects and shaders
	auto pbrShader		= gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
	auto skyBoxShader	= gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "SkyBox.vert" ) ).fragment( loadAsset( "SkyBox.frag" ) ) );
	mModelBatch			= gl::Batch::create( geom::Sphere().subdivisions( 32 ), pbrShader );
	mSkyBoxBatch		= gl::Batch::create( geom::Cube().size( vec3( 500 ) ), skyBoxShader );
	
	// load the prefiltered IBL Cubemaps
	auto cubeMapFormat	= gl::TextureCubeMap::Format().mipmap().internalFormat( GL_RGB16F ).minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR );
	mIrradianceMap		= gl::TextureCubeMap::createFromDds( loadAsset( "WellsIrradiance.dds" ), cubeMapFormat );
	mRadianceMap		= gl::TextureCubeMap::createFromDds( loadAsset( "WellsRadiance.dds" ), cubeMapFormat );
	
	// set the initial parameters and setup the ui
	mGridSize			= 5;
	mRoughness			= 1.0f;
	mMetallic			= 1.0f;
	mSpecular			= 1.0f;
	mBaseColor			= Color::white();
	mGamma				= 2.2f;
	mExposure			= 4.0f;
	mTime				= 0.0f;
	mShowUi				= false;
	mRotateModel		= false;
	
	// prepare ui
	ui::initialize();
	getWindow()->getSignalKeyDown().connect( [this]( KeyEvent event ) {
		if( event.getCode() == KeyEvent::KEY_SPACE ) mShowUi = !mShowUi;
	} );
}

void PBRImageBasedLightingApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void PBRImageBasedLightingApp::update()
{
	// user interface
	if( mShowUi ) {
		ui::ScopedWindow window( "PBRBasics" );
		if( ui::CollapsingHeader( "Material", nullptr, true, true ) ) {
			ui::DragFloat( "Roughness", &mRoughness, 0.01f, 0.0f, 1.0f );
			ui::DragFloat( "Metallic", &mMetallic, 0.01f, 0.0f, 1.0f );
			ui::DragFloat( "Specular", &mSpecular, 0.01f, 0.0f, 1.0f );
			ui::ColorEdit3( "Color", &mBaseColor[0] );
		}
		if( ui::CollapsingHeader( "Model", nullptr, true, true ) ) {
			static int currentPrimitive = 0;
			const static vector<string> primitives = { "Sphere", "Teapot", "Cube", "Capsule", "Torus", "TorusKnot" };
			if( ui::Combo( "Primitive", &currentPrimitive, primitives ) ) {
				switch ( currentPrimitive ) {
					case 0: mModelBatch->replaceVboMesh( gl::VboMesh::create( geom::Sphere().subdivisions( 32 ) ) ); break;
					case 1: mModelBatch->replaceVboMesh( gl::VboMesh::create( geom::Teapot().subdivisions( 16 ) >> geom::Transform( glm::scale( vec3( 1.5f ) ) ) ) ); break;
					case 2: mModelBatch->replaceVboMesh( gl::VboMesh::create( geom::Cube() ) ); break;
					case 3: mModelBatch->replaceVboMesh( gl::VboMesh::create( geom::Capsule().subdivisionsAxis( 32 ).subdivisionsHeight( 32 ) ) ); break;
					case 4: mModelBatch->replaceVboMesh( gl::VboMesh::create( geom::Torus().subdivisionsAxis( 32 ).subdivisionsHeight( 32 ) ) ); break;
					case 5: mModelBatch->replaceVboMesh( gl::VboMesh::create( geom::TorusKnot().subdivisionsAxis( 128 ).subdivisionsHeight( 128 ).scale( vec3( 0.5f ) ) ) ); break;
				}
			}
			ui::Checkbox( "Rotate", &mRotateModel );
		}
		if( ui::CollapsingHeader( "Environment", nullptr, true, true ) ) {
			static int currentEnvironment = 1;
			const static vector<string> environments = { "Bolonga", "Wells", "Cathedral" };
			if( ui::Combo( "###Environments", &currentEnvironment, environments ) ) {
				auto cubeMapFormat	= gl::TextureCubeMap::Format().mipmap().internalFormat( GL_RGB16F ).minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR );
				mIrradianceMap		= gl::TextureCubeMap::createFromDds( loadAsset( environments[currentEnvironment] + "Irradiance.dds" ), cubeMapFormat );
				mRadianceMap		= gl::TextureCubeMap::createFromDds( loadAsset( environments[currentEnvironment] + "Radiance.dds" ), cubeMapFormat );
			}
		}
		if( ui::CollapsingHeader( "Rendering", nullptr, true, true ) ) {
			ui::DragFloat( "Gamma", &mGamma, 0.01f, 0.0f );
			ui::DragFloat( "Exposure", &mExposure, 0.01f, 0.0f );
		}
	}
	
	if( mRotateModel ){
		mTime += 0.025f;
	}
}

void PBRImageBasedLightingApp::draw()
{
	// clear window and set matrices
	gl::clear( Color( 1, 0, 0 ) );
	gl::setMatrices( mCamera );
	
	// enable depth testing
	gl::ScopedDepth scopedDepth( true );
	
	// bind the cubemap textures
	gl::ScopedTextureBind scopedTexBind0( mRadianceMap, 0 );
	gl::ScopedTextureBind scopedTexBind1( mIrradianceMap, 1 );
	auto shader = mModelBatch->getGlslProg();
	shader->uniform( "uRadianceMap", 0 );
	shader->uniform( "uIrradianceMap", 1 );
	
	// sends the base color, the specular opacity,
	// the light position, color and radius to the shader
	shader->uniform( "uBaseColor", mBaseColor );
	shader->uniform( "uSpecular", mSpecular );
	
	// sends the tone-mapping uniforms
	shader->uniform( "uExposure", mExposure );
	shader->uniform( "uGamma", mGamma );
	
	// render a grid of sphere with different roughness/metallic values and colors
	{
		gl::ScopedMatrices scopedMatrices;
		for( int x = -mGridSize; x <= mGridSize; x++ ){
			for( int z = -mGridSize; z <= mGridSize; z++ ){
				float roughness = lmap( (float) z, (float) -mGridSize, (float) mGridSize, 0.02f, 1.0f );
				float metallic	= lmap( (float) x, (float) -mGridSize, (float) mGridSize, 1.0f, 0.0f );
				
				shader->uniform( "uRoughness", roughness * mRoughness );
				shader->uniform( "uRoughness4", pow( roughness * mRoughness, 4.0f ) );
				shader->uniform( "uMetallic", metallic * mMetallic );
				
				gl::setModelMatrix( glm::translate( vec3( x, 0, z ) * 2.25f ) * glm::rotate( mTime, vec3( 0.123, 0.456, 0.789 ) ) );
				mModelBatch->draw();
			}
		}
	}
	
	// render skybox
	shader = mSkyBoxBatch->getGlslProg();
	shader->uniform( "uExposure", mExposure );
	shader->uniform( "uGamma", mGamma );
	mSkyBoxBatch->draw();
}

CINDER_APP( PBRImageBasedLightingApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
/*
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Shader.h"

#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PBRImageBasedLightingApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void resize() override;
	void keyDown( KeyEvent event ) override;
	
	void setupParams();
	void prefilterCubeMap();
	
	CameraPersp				mCamera;
	CameraUi				mCameraUi;
	gl::GlslProgRef			mShader, mColorShader, mSkyBoxShader, mCubeMapPrefiltering;
	params::InterfaceGlRef	mParams;
	
	gl::FboCubeMapRef		mFilteredCubeMap;
	
	gl::VboMeshRef			mModel;
	gl::VboMeshRef			mSkyBox;
	
	float					mRoughness;
	float					mMetallic;
	float					mSpecular;
	Color					mBaseColor;
	
	gl::TextureCubeMapRef	mCubeMap;
	
	bool					mDisplayBackground;
	bool					mRotateModel;
	float					mTime;
	
	float					mGamma;
};

void PBRImageBasedLightingApp::setup()
{
	// build our test model and skybox
	mModel	= gl::VboMesh::create( geom::Sphere().subdivisions( 128 ) );
	mSkyBox = gl::VboMesh::create( geom::Cube() );
	
	// load the cubemap texture
	mCubeMap = gl::TextureCubeMap::create( loadImage( loadAsset( "grimmnight_large.jpg" ) ) );
	
	// create a Camera and a Camera ui
	mCamera = CameraPersp();
	mCamera.setPerspective( 50.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	mCamera.setEyePoint( vec3( -37.653, 40.849, -0.187 ) );
	mCamera.setOrientation( quat( -0.643, 0.298, 0.640, 0.297 ) );
	mCameraUi = CameraUi( &mCamera, getWindow() );
	
	// compile the shader and make sure they compile fine
	try {
		mShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
		mShader->uniform( "uCubeMapTex", 0 );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	try {
		mSkyBoxShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "sky_box.vert" ) ).fragment( loadAsset( "sky_box.frag" ) ) );
		mSkyBoxShader->uniform( "uCubeMapTex", 0 );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	
	
	mFilteredCubeMap = gl::FboCubeMap::create( 512, 512, gl::FboCubeMap::Format().textureCubeMapFormat( gl::TextureCubeMap::Format().internalFormat( GL_RGB16F ).minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR_MIPMAP_LINEAR ).mipmap() ) );
	
	try {
		mCubeMapPrefiltering = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PrefilterEnvMap.vert" ) ).fragment( loadAsset( "PrefilterEnvMap.frag" ) ) );
		mCubeMapPrefiltering->uniform( "uCubeMapTex", 0 );
		prefilterCubeMap();
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	
	
	// create a shader for rendering the light
	mColorShader = gl::getStockShader( gl::ShaderDef().color() );
	
	// set the initial parameters and setup the ui
	mRoughness			= 1.0f;
	mMetallic			= 1.0f;
	mSpecular			= 1.0f;
	mBaseColor			= Color::white();//( 1.0f, 0.8f, 0.025f );
	mTime				= 0.0f;
	mRotateModel		= false;
	mDisplayBackground	= true;
	mFStop				= 2.0f;
	mGamma				= 2.2f;
	mFocalLength		= 36.0f;
	mSensorSize			= 35.0f;
	mFocalLengthPreset	= mPrevFocalLengthPreset = 3;
	mSensorSizePreset	= mPrevSensorSizePreset = 4;
	mFStopPreset		= mPrevFStopPreset = 2;
	
	setupParams();
}

void PBRImageBasedLightingApp::prefilterCubeMap()
{
	auto sphere = gl::VboMesh::create( geom::Sphere().subdivisions( 128 ).radius( 10.0f ) );
	
	gl::ScopedTextureBind texScp( mCubeMap );
	gl::ScopedGlslProg shaderScp( mCubeMapPrefiltering );
	gl::ScopedFramebuffer framebufferScp( mFilteredCubeMap );
	gl::ScopedBlend disableBlending( false );
	
	int numMipMaps = 6;
	ivec2 size = mFilteredCubeMap->getSize();
	mCubeMapPrefiltering->uniform( "uMaxLod", (float) numMipMaps );
	mCubeMapPrefiltering->uniform( "uSize", (float) size.x );
	
	for( int level = 0; level < numMipMaps; level++ ){
		gl::ScopedViewport viewport( ivec2( 0, 0 ), size );
		for( uint8_t dir = 0; dir < 6; ++dir ) {
			gl::setProjectionMatrix( ci::CameraPersp( size.x, size.y, 90.0f, 0.1f, 1000 ).getProjectionMatrix() );
			gl::setViewMatrix( mFilteredCubeMap->calcViewMatrix( GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir, vec3( 0 ) ) );
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir, mFilteredCubeMap->getTextureCubeMap()->getId(), level );
			
			mCubeMapPrefiltering->uniform( "uLod", (float) level );
			
			gl::clear();
			gl::draw( sphere );
		}
		size /= 2;
	}
}

static vector<string>	sFocalPresets			= { "Custom(mm)", "Super Wide(15mm)", "Wide Angle(25mm)", "Classic(36mm)", "Normal Lense(50mm)" };
static vector<float>	sFocalPresetsValues		= { -1.0f, 15.0f, 25.0f, 36.0f, 50.0f };
static vector<string>	sSensorPresets			= { "Custom(mm)", "8mm", "16mm", "22mm", "35mm", "70mm" };
static vector<float>	sSensorPresetsValues	= { -1.0f, 8.0f, 16.0f, 22.0f, 35.0f, 70.0f };
static vector<string>	sFStopPresets			= { "Custom", "f/1.4", "f/2", "f/2.8", "f/4", "f/5.6", "f/8" };
static vector<float>	sFStopPresetsValues		= { -1.0f, 1.4f, 2.0f, 2.8f, 4.0f, 5.6f, 8.0f };

void PBRImageBasedLightingApp::update()
{
	if( mRotateModel ){
		mTime += 0.025f;
	}
	
	// check for camera properties change
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
	if( mCamera.getFov() != fov ){
		mCamera.setFov( fov );
	}
	
	getWindow()->setTitle( "Fps: " + to_string( (int) getAverageFps() ) );
}

void PBRImageBasedLightingApp::draw()
{
	
	gl::clear( Color( 0, 0, 0 ) );
	
	// set matrices
	gl::setMatrices( mCamera );
	
	// enable depth testing
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	{
		// set the shader and bind the cubemap texture
		gl::ScopedGlslProg shadingScp( mShader );
		gl::ScopedTextureBind cubeMapScp( mFilteredCubeMap->getTextureCubeMap() );
		
		// sends the base color, the specular opacity,
		// the light position, color and radius to the shader
		mShader->uniform( "uBaseColor", mBaseColor );
		mShader->uniform( "uSpecular", mSpecular );
		
		// sends the tone-mapping uniforms
		// ( the 0.0f / 144.0f and 0.1f / 50.0f ranges are totally arbitrary )
		mShader->uniform( "uExposure", lmap( mFocalLength / mFStop, 0.0f, 144.0f, 0.1f, 50.0f ) );
		mShader->uniform( "uGamma", mGamma );
		
		
		// render a grid of sphere with different roughness/metallic values and colors
		int gridSize = 5;
		gl::pushModelMatrix();
		for( int x = -gridSize; x <= gridSize; x++ ){
			for( int z = -gridSize; z <= gridSize; z++ ){
				float roughness = lmap( (float) z, (float) -gridSize, (float) gridSize, 0.05f, 1.0f );
				float metallic	= lmap( (float) x, (float) -gridSize, (float) gridSize, 1.0f, 0.0f );
				
				mShader->uniform( "uRoughness", 0.0001f + roughness * mRoughness );
				mShader->uniform( "uRoughness4", pow( 0.0001f + roughness * mRoughness, 4.0f ) );
				mShader->uniform( "uMetallic", metallic * mMetallic );
				
				gl::setModelMatrix( glm::translate( vec3( x, 0, z ) * 2.5f ) * glm::rotate( mTime, vec3( 0.123, 0.456, 0.789 ) ) );
				gl::draw( mModel );
			}
		}
		gl::popModelMatrix();
		
		// render the skybox
		if( mDisplayBackground ){
			//gl::ScopedTextureBind cubeMapScp( mCubeMap );
			gl::ScopedGlslProg skyBoxShaderScp( mSkyBoxShader );
			
			mSkyBoxShader->uniform( "uExposure", lmap( mFocalLength / mFStop, 0.0f, 144.0f, 0.1f, 50.0f ) );
			mSkyBoxShader->uniform( "uGamma", mGamma );
			
			gl::pushMatrices();
			gl::scale( vec3( 150.0f ) );
			gl::draw( mSkyBox );
			gl::popMatrices();
		}
	}
	
	// render the ui
	mParams->draw();
}

void PBRImageBasedLightingApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() ) {
		case KeyEvent::KEY_1:
			mModel = gl::VboMesh::create( geom::Sphere().subdivisions( 32 ) );
		break;
		case KeyEvent::KEY_2:
			mModel = gl::VboMesh::create( geom::Teapot().subdivisions( 16 ) >> geom::Transform( glm::scale( vec3( 1.5f ) ) ) );
		break;
		case KeyEvent::KEY_3:
			mModel = gl::VboMesh::create( geom::Cube() );
		break;
		case KeyEvent::KEY_4:
			mModel = gl::VboMesh::create( geom::Capsule().subdivisionsAxis( 32 ).subdivisionsHeight( 32 ) );
		break;
		case KeyEvent::KEY_5:
			mModel = gl::VboMesh::create( geom::Torus().subdivisionsAxis( 32 ).subdivisionsHeight( 32 ) );
		break;
		case KeyEvent::KEY_6:
			mModel = gl::VboMesh::create( geom::Cone().subdivisionsAxis( 32 ).subdivisionsHeight( 32 ) );
		break;
		case KeyEvent::KEY_SPACE:
			mRotateModel = !mRotateModel;
		break;
		case KeyEvent::KEY_q:
			mCubeMap = gl::TextureCubeMap::create( loadImage( loadAsset( "env_map.jpg" ) ) );
			prefilterCubeMap();
		break;
		case KeyEvent::KEY_w:
			mCubeMap = gl::TextureCubeMap::create( loadImage( loadAsset( "env_map2.jpg" ) ) );
			prefilterCubeMap();
		break;
		case KeyEvent::KEY_e:
			mCubeMap = gl::TextureCubeMap::create( loadImage( loadAsset( "grimmnight_large.jpg" ) ) );
			prefilterCubeMap();
		break;
		case KeyEvent::KEY_r:
			mCubeMap = gl::TextureCubeMap::create( loadImage( loadAsset( "interstellar_large.jpg" ) ) );
			prefilterCubeMap();
		break;
		case KeyEvent::KEY_t:
			mCubeMap = gl::TextureCubeMap::create( loadImage( loadAsset( "violentdays_large.jpg" ) ) );
			prefilterCubeMap();
		break;
	}
}
void PBRImageBasedLightingApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void PBRImageBasedLightingApp::setupParams()
{
	mParams = params::InterfaceGl::create( "PBR", ivec2( 300, 400 ) );
	mParams->minimize();
	
	mParams->addText( "Material" );
	mParams->addParam( "Roughness", &mRoughness ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Metallic", &mMetallic ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Specular", &mSpecular ).min( 0.0f ).max( 1.0f ).step( 0.01f );
	mParams->addParam( "Base Color", &mBaseColor );
	
	mParams->addSeparator();
	mParams->addText( "Animation" );
	mParams->addParam( "Rotate Model", &mRotateModel );
	
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
	
	mParams->addSeparator();
	mParams->addText( "Other" );
	mParams->addParam( "Display Background", &mDisplayBackground );
}

CINDER_APP( PBRImageBasedLightingApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
*/