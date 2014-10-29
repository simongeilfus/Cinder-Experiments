#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Query.h"

#include "cinder/MayaCamUI.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"

#include "AssetManager.h"

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
	void keyDown( KeyEvent event ) override;
	
	void setupParams();
	void prefilterCubeMap();
	
	MayaCamUI				mMayaCam;
	gl::GlslProgRef			mShader, mColorShader, mSkyBoxShader, mCubeMapPrefiltering;
	params::InterfaceGlRef	mParams;
	
	gl::FboCubeMapRef		mFilteredCubeMap;
	
	gl::VboMeshRef			mModel;
	gl::VboMeshRef			mSkyBox;
	
	gl::QueryTimeSwappedRef	mTimer;
	
	float					mRoughness;
	float					mMetallic;
	float					mSpecular;
	Color					mBaseColor;
	
	gl::TextureCubeMapRef	mCubeMap;
	
	bool					mDisplayBackground;
	bool					mRotateModel;
	float					mTime;
	
	float					mFocalLength, mSensorSize, mFStop;
	int						mFocalLengthPreset, mSensorSizePreset, mFStopPreset;
	int						mPrevFocalLengthPreset, mPrevSensorSizePreset, mPrevFStopPreset;
	float					mGamma;
	
	Font					mFont;
};

void PhysicallyBasedShadingApp::setup()
{
	// build our test model and skybox
	mModel	= gl::VboMesh::create( /*geom::Teapot() );*/ geom::Sphere().subdivisions( 128 ) );
	mSkyBox = gl::VboMesh::create( geom::Cube() );
	
	// load the cubemap texture
	const string cubemapName = "uffizi";
	const string cubemapExt = "tif";
	const ImageSourceRef cubemap[6] = {
		loadImage( loadAsset( cubemapName + "0003." + cubemapExt ) ),
		loadImage( loadAsset( cubemapName + "0002." + cubemapExt ) ),
		loadImage( loadAsset( cubemapName + "0004." + cubemapExt ) ),
		loadImage( loadAsset( cubemapName + "0005." + cubemapExt ) ),
		loadImage( loadAsset( cubemapName + "0000." + cubemapExt ) ),
		loadImage( loadAsset( cubemapName + "0001." + cubemapExt ) )
	};
	mCubeMap = gl::TextureCubeMap::create( cubemap, gl::TextureCubeMap::Format().internalFormat( GL_RGB32F ).mipmap() );
	
	//mCubeMap = gl::TextureCubeMap::createHorizontalCross( loadImage( loadAsset( "interstellar_large.jpg" ) ) );
	
	
	// prepare the Camera ui
	auto cam = CameraPersp();
	cam.setPerspective( 50.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
	cam.setEyePoint( vec3( -37.653, 40.849, -0.187 ) );
	cam.setOrientation( quat( -0.643, 0.298, 0.640, 0.297 ) );
	mMayaCam.setCurrentCam( cam );
	
	// load and compile the shader and make sure they compile fine
	AssetManager::load( "PBR.vert", "PBR.frag", [this]( DataSourceRef vert, DataSourceRef frag ){
		
		try {
//			mShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
			mShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( vert ).fragment( frag ) );
			mShader->uniform( "uCubeMapTex", 0 );
		}
		catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	} );
	
	try {
		mSkyBoxShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "sky_box.vert" ) ).fragment( loadAsset( "sky_box.frag" ) ) );
		mSkyBoxShader->uniform( "uCubeMapTex", 0 );
	}
	catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	
	
	mFilteredCubeMap = gl::FboCubeMap::create( 512, 512, gl::FboCubeMap::Format().textureCubeMapFormat( gl::TextureCubeMap::Format().internalFormat( GL_RGB16F ).mipmap().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR_MIPMAP_LINEAR ) ) );
	
	
	AssetManager::load( "PrefilterEnvMap.vert", "PrefilterEnvMap.frag", [this]( DataSourceRef vert, DataSourceRef frag ){
		
		try {
			mCubeMapPrefiltering = gl::GlslProg::create( gl::GlslProg::Format().vertex( vert ).fragment( frag ) );
			mCubeMapPrefiltering->uniform( "uCubeMapTex", 0 );
			prefilterCubeMap();
		}
		catch( gl::GlslProgCompileExc exc ){ CI_LOG_E( exc.what() ); }
	} );
	
	mTimer = gl::QueryTimeSwapped::create();
	
	// create a shader for rendering the light
	mColorShader = gl::getStockShader( gl::ShaderDef().color() );
	
	// set the initial parameters and setup the ui
	mRoughness			= 1.0f;
	mMetallic			= 1.0f;
	mSpecular			= 1.0f;
	mBaseColor			= Color::white();//( 1.0f, 0.8f, 0.025f );
	mTime				= 0.0f;
	mRotateModel		= false;
	mDisplayBackground	= false;
	mFStop				= 2.0f;
	mGamma				= 2.2f;
	mFocalLength		= 36.0f;
	mSensorSize			= 35.0f;
	mFocalLengthPreset	= mPrevFocalLengthPreset = 3;
	mSensorSizePreset	= mPrevSensorSizePreset = 4;
	mFStopPreset		= mPrevFStopPreset = 2;
	
	setupParams();
	
#if defined( CINDER_MSW )
	mFont = Font( "Arial Bold", 12 );
#else
	mFont = Font( "Arial-BoldMT", 12 );
#endif
}

void PhysicallyBasedShadingApp::prefilterCubeMap()
{
	auto sphere = gl::VboMesh::create( geom::Sphere().subdivisions( 128 ) );
	
	gl::ScopedTextureBind texScp( mCubeMap );
	gl::ScopedGlslProg shaderScp( mCubeMapPrefiltering );
	gl::ScopedFramebuffer framebufferScp( mFilteredCubeMap );
	
 
 
	int numMipMaps = 6;
	ivec2 size = mFilteredCubeMap->getSize();
	mCubeMapPrefiltering->uniform( "uMaxLod", (float) numMipMaps );
	mCubeMapPrefiltering->uniform( "uSize", (float) size.x );
	
	for( int level = 0; level < numMipMaps; level++ ){
		
		gl::pushViewport( ivec2( 0, 0 ), size );
		for( uint8_t dir = 0; dir < 6; ++dir ) {
			gl::setProjectionMatrix( ci::CameraPersp( size.x, size.y, 90.0f, 1, 1000 ).getProjectionMatrix() );
			gl::setViewMatrix( mFilteredCubeMap->calcViewMatrix( GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir, vec3( 0 ) ) );
			
			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + dir, mFilteredCubeMap->getTexture( GL_COLOR_ATTACHMENT0 )->getId(), level );
			
			mCubeMapPrefiltering->uniform( "uLod", (float) level );
			
			gl::clear();
			gl::pushMatrices();
			gl::scale( vec3( 10.0f ) );
			gl::draw( sphere );
			gl::popMatrices();
		}
		gl::popViewport();
		
		CI_LOG_V( "Mipmap level " << level << " generated at " << size );
		size /= 2;
	}
}

void PhysicallyBasedShadingApp::update()
{
	if( mRotateModel ){
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
	
	getWindow()->setTitle( "Fps: " + to_string( (int) getAverageFps() ) );// + " / " + to_string( mTimer->getElapsedMilliseconds() ) + "ms" );
}

void PhysicallyBasedShadingApp::draw()
{
	//mTimer->begin();
	
	gl::clear( Color( 0, 0, 0 ) );
	
	// set matrices
	gl::setMatrices( mMayaCam.getCamera() );
	
	// enable depth testing
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	{
		// set the shader and bind the cubemap texture
		gl::ScopedGlslProg shadingScp( mShader );
		gl::ScopedTextureBind cubeMapScp( mFilteredCubeMap->getTexture( GL_COLOR_ATTACHMENT0 ) );
		
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
			gl::ScopedGlslProg skyBoxShaderScp( mSkyBoxShader );
			
			mSkyBoxShader->uniform( "uExposure", lmap( mFocalLength / mFStop, 0.0f, 144.0f, 0.1f, 50.0f ) );
			mSkyBoxShader->uniform( "uGamma", mGamma );
			
			gl::pushMatrices();
			gl::scale( vec3( 150.0f ) );
			gl::draw( mSkyBox );
			gl::popMatrices();
		}
	}
	
	
	//mTimer->end();
	
	// render the ui
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

void PhysicallyBasedShadingApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() ) {
  case KeyEvent::KEY_1:
			mModel = gl::VboMesh::create( geom::Sphere().subdivisions( 32 ) );
			break;
  case KeyEvent::KEY_2:
			mModel = gl::VboMesh::create( geom::Transform( geom::Teapot().subdivisions( 16 ), glm::scale( vec3( 1.5f ) ) ) );
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
  case KeyEvent::KEY_q:
			mCubeMap = gl::TextureCubeMap::createHorizontalCross( loadImage( loadAsset( "env_map.jpg" ) ) );
			prefilterCubeMap();
			break;
  case KeyEvent::KEY_w:
			mCubeMap = gl::TextureCubeMap::createHorizontalCross( loadImage( loadAsset( "env_map2.jpg" ) ) );
			prefilterCubeMap();
			break;
  case KeyEvent::KEY_e:
			mCubeMap = gl::TextureCubeMap::createHorizontalCross( loadImage( loadAsset( "grimmnight_large.jpg" ) ) );
			prefilterCubeMap();
			break;
  case KeyEvent::KEY_r:
			mCubeMap = gl::TextureCubeMap::createHorizontalCross( loadImage( loadAsset( "interstellar_large.jpg" ) ) );
			prefilterCubeMap();
			break;
  case KeyEvent::KEY_t:
			mCubeMap = gl::TextureCubeMap::createHorizontalCross( loadImage( loadAsset( "violentdays_large.jpg" ) ) );
			prefilterCubeMap();
			break;
  case KeyEvent::KEY_y:
			const string cubemapName = "uffizi";
			const string cubemapExt = "tif";
			const ImageSourceRef cubemap[6] = {
				loadImage( loadAsset( cubemapName + "0003." + cubemapExt ) ),
				loadImage( loadAsset( cubemapName + "0002." + cubemapExt ) ),
				loadImage( loadAsset( cubemapName + "0004." + cubemapExt ) ),
				loadImage( loadAsset( cubemapName + "0005." + cubemapExt ) ),
				loadImage( loadAsset( cubemapName + "0000." + cubemapExt ) ),
				loadImage( loadAsset( cubemapName + "0001." + cubemapExt ) )
			};
			mCubeMap = gl::TextureCubeMap::create( cubemap, gl::TextureCubeMap::Format().internalFormat( GL_RGB32F ).mipmap() );
			prefilterCubeMap();
			break;
	}
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

CINDER_APP_NATIVE( PhysicallyBasedShadingApp, RendererGl )
