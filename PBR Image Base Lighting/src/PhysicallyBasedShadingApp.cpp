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
	
	vec3					mLightPosition;
	MayaCamUI				mMayaCam;
	gl::GlslProgRef			mShader, mColorShader, mSkyBoxShader;
	params::InterfaceGlRef	mParams;
	
	gl::VboMeshRef			mModel;
	gl::VboMeshRef			mSkyBox;
	
	float					mRoughness;
	float					mMetallic;
	float					mSpecular;
	Color					mBaseColor;
	
	gl::TextureCubeMapRef	mCubeMap;
	
	bool					mAnimateLight;
	float					mLightRadius;
	Color					mLightColor;
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
	mModel	= gl::VboMesh::create( /*geom::Teapot() );*/ geom::Sphere().subdivisions( 32 ) );
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
	
	
	// create a shader for rendering the light
	mColorShader = gl::getStockShader( gl::ShaderDef().color() );
	
	// set the initial parameters and setup the ui
	mRoughness			= 1.0f;
	mMetallic			= 1.0f;
	mSpecular			= 1.0f;
	mLightRadius		= 2.0f;
	mLightColor			= Color::white();//( 1.0f, 0.025f, 0.9f );
	mBaseColor			= Color::white();//( 1.0f, 0.8f, 0.025f );
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
	
#if defined( CINDER_MSW )
	mFont = Font( "Arial Bold", 12 );
#else
	mFont = Font( "Arial-BoldMT", 12 );
#endif
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
	
	// set matrices
	gl::setMatrices( mMayaCam.getCamera() );
	
	// enable depth testing
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	{
		// set the shader and bind the cubemap texture
		gl::ScopedGlslProg shadingScp( mShader );
		gl::ScopedTextureBind cubeMapScp( mCubeMap );
		
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
		int gridSize = 4;
		gl::pushModelMatrix();
		for( int x = -gridSize; x <= gridSize; x++ ){
			for( int z = -gridSize; z <= gridSize; z++ ){
				float roughness = lmap( (float) z, (float) -gridSize, (float) gridSize, 0.05f, 1.0f );
				float metallic	= lmap( (float) x, (float) -gridSize, (float) gridSize, 1.0f, 0.0f );
				
				mShader->uniform( "uRoughness", 0.0001f + pow( roughness * mRoughness, 4.0f ) );
				mShader->uniform( "uMetallic", metallic * mMetallic );
				
				gl::setModelMatrix( glm::translate( vec3( x, 0, z ) * 2.5f ) );
				gl::draw( mModel );
			}
		}
		gl::popModelMatrix();
		
		// render the skybox
		gl::ScopedGlslProg skyBoxShaderScp( mSkyBoxShader );
		
		mSkyBoxShader->uniform( "uExposure", lmap( mFocalLength / mFStop, 0.0f, 144.0f, 0.1f, 50.0f ) );
		mSkyBoxShader->uniform( "uGamma", mGamma );
		
		gl::pushMatrices();
		gl::scale( vec3( 150.0f ) );
		gl::draw( mSkyBox );
		gl::popMatrices();
	}
	
	
	
	
	/*// render the light
	gl::ScopedGlslProg colorShaderScp( mColorShader );
	gl::color( mLightColor + Color::white() * 0.5f );
	gl::drawSphere( mLightPosition, mLightRadius * 0.15f, 32.0f );*/
	
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
			mModel = gl::VboMesh::create( geom::Capsule().subdivisionsAxis( 32 ) );
			break;
  case KeyEvent::KEY_5:
			mModel = gl::VboMesh::create( geom::Torus().subdivisionsAxis( 32 ) );
			break;
  case KeyEvent::KEY_6:
			mModel = gl::VboMesh::create( geom::Cone() );
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
