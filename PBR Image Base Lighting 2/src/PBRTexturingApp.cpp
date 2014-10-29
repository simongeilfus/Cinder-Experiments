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

#include "AssetManager.h"

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
	void keyDown( KeyEvent event ) override;
	
	void setupParams();
	void prefilterCubeMap();
	
	MayaCamUI				mMayaCam;
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
	
	float					mFocalLength, mSensorSize, mFStop;
	int						mFocalLengthPreset, mSensorSizePreset, mFStopPreset;
	int						mPrevFocalLengthPreset, mPrevSensorSizePreset, mPrevFStopPreset;
	float					mGamma;
	
	gl::TextureRef			mCopyrightMap;
	gl::Texture2dRef		mBaseColorMap;
	gl::Texture2dRef		mNormalMap;
	gl::Texture2dRef		mRoughnessMap;
	gl::Texture2dRef		mMetallicMap;
	gl::Texture2dRef		mEmissiveMap;
	
	Perlin					mPerlin;
	float					mNormalDetails;
	bool					mAnimateLight;
	
	vector<vec3>			mLightPositions;
	vector<float>			mLightRadiuses;
	vector<Color>			mLightColors;
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
	mModel	= gl::VboMesh::create( mesh );
	
	// build the skybox
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
	
	// and the model textures
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


void PBRTexturingApp::prefilterCubeMap()
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
	
	getWindow()->setTitle( "Fps: " + to_string( (int) getAverageFps() ) );
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
		// set the shader and bind the cubemap texture
		gl::ScopedGlslProg shadingScp( mShader );
		gl::ScopedTextureBind cubeMapScp( mFilteredCubeMap->getTexture( GL_COLOR_ATTACHMENT0 ) );
		
		// upload the shader uniforms
		mShader->uniform( "uLightPositions", &mLightPositions[0], mLightPositions.size() );
		vector<vec3> colors;
		colors.push_back( vec3( mLightColors[0].r, mLightColors[0].g, mLightColors[0].b ) );
		colors.push_back( vec3( mLightColors[1].r, mLightColors[1].g, mLightColors[1].b ) );
		mShader->uniform( "uLightColors", &colors[0], mLightPositions.size() );
		mShader->uniform( "uLightRadiuses", &mLightRadiuses[0], mLightPositions.size() );
		mShader->uniform( "uBaseColor", mBaseColor );
		mShader->uniform( "uSpecular", mSpecular );
		mShader->uniform( "uRoughness", mRoughness );
		mShader->uniform( "uRoughness4", pow( mRoughness, 4.0f ) );
		mShader->uniform( "uMetallic", mMetallic );
		mShader->uniform( "uDetails", mNormalDetails );
		mShader->uniform( "uExposure", lmap( mFocalLength / mFStop, 0.0f, 144.0f, 0.1f, 50.0f ) );
		mShader->uniform( "uGamma", mGamma );
		mShader->uniform( "uMask", false );
		mShader->uniform( "uDisplayBackground", mDisplayBackground );
		
		// bind textures and send their id's to the shader
		glActiveTexture( GL_TEXTURE1 );
		gl::ScopedTextureBind baseColorMapScp( mBaseColorMap );
		mShader->uniform( "uBaseColorMap", 1 );
		
		glActiveTexture( GL_TEXTURE2 );
		gl::ScopedTextureBind normalMapScp( mNormalMap );
		mShader->uniform( "uNormalMap", 2 );
		
		glActiveTexture( GL_TEXTURE3 );
		gl::ScopedTextureBind roughnessMapScp( mRoughnessMap );
		mShader->uniform( "uRoughnessMap", 3 );
		
		glActiveTexture( GL_TEXTURE4 );
		gl::ScopedTextureBind metallicMapScp( mMetallicMap );
		mShader->uniform( "uMetallicMap", 4 );
		
		glActiveTexture( GL_TEXTURE5 );
		gl::ScopedTextureBind aoMapScp( mEmissiveMap );
		mShader->uniform( "uEmissiveMap", 5 );
		
		glActiveTexture( GL_TEXTURE0 );
		
		// render model
		gl::pushModelMatrix();
		
		// scale down
		gl::setModelMatrix( glm::scale( mat4(), vec3( 0.25f ) ) );
		gl::color( Color::white() );
		gl::draw( mModel );
		gl::popModelMatrix();
		
		// render a sphere so we got a background
		// this is needed by the scattering code
		gl::disableDepthWrite();
		
		mShader->uniform( "uMask", true );
		
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

void PBRTexturingApp::keyDown( KeyEvent event )
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
	
	mParams->addSeparator();
	mParams->addText( "Other" );
	mParams->addParam( "Display Background", &mDisplayBackground );
}

CINDER_APP_NATIVE( PBRTexturingApp, RendererGl( RendererGl::Options().antiAliasing( RendererGl::AA_MSAA_16 ) ) )
