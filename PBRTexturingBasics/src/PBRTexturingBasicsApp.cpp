#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PBRTexturingBasicsApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void resize() override;
	
	CameraPersp				mCamera;
	CameraUi				mCameraUi;
	gl::BatchRef			mModelBatch, mSkyBoxBatch;
	gl::TextureCubeMapRef	mIrradianceMap, mRadianceMap;
	gl::Texture2dRef		mNormalMap, mRoughnessMap, mMetallicMap;
	
	int						mGridSize;
	bool					mShowUi, mRotateModel;
	float					mRoughness, mMetallic, mSpecular;
	Color					mBaseColor;
	float					mGamma, mExposure, mTime;
};

void PBRTexturingBasicsApp::setup()
{
	// create a Camera and a Camera ui
	mCamera		= CameraPersp( getWindowWidth(), getWindowHeight(), 50.0f, 0.1f, 200.0f ).calcFraming( Sphere( vec3( 0.0f ), 2.0f ) );
	mCameraUi	= CameraUi( &mCamera, getWindow(), -1 );
	
	// prepare ou rendering objects and shaders
	auto pbrShader		= gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
	auto skyBoxShader	= gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "SkyBox.vert" ) ).fragment( loadAsset( "SkyBox.frag" ) ) );
	mModelBatch			= gl::Batch::create( geom::Teapot().subdivisions( 16 ) >> geom::Transform( glm::scale( vec3( 1.5f ) ) ), pbrShader );
	mSkyBoxBatch		= gl::Batch::create( geom::Cube().size( vec3( 100 ) ), skyBoxShader );
	
	// load the prefiltered IBL Cubemaps
	auto cubeMapFormat	= gl::TextureCubeMap::Format().mipmap().internalFormat( GL_RGB16F ).minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR );
	mIrradianceMap		= gl::TextureCubeMap::createFromDds( loadAsset( "WellsIrradiance.dds" ), cubeMapFormat );
	mRadianceMap		= gl::TextureCubeMap::createFromDds( loadAsset( "WellsRadiance.dds" ), cubeMapFormat );
	
	// load the material textures
	auto textureFormat	= gl::Texture2d::Format().mipmap().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR );
	mNormalMap			= gl::Texture2d::create( loadImage( loadAsset( "normal.png" ) ) );
	mRoughnessMap		= gl::Texture2d::create( loadImage( loadAsset( "roughness.png" ) ), textureFormat );
	mMetallicMap		= gl::Texture2d::create( loadImage( loadAsset( "metallic.png" ) ), textureFormat );
	
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

void PBRTexturingBasicsApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void PBRTexturingBasicsApp::update()
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
			static int currentPrimitive = 1;
			const static vector<string> primitives = { "Sphere", "Teapot", "Cube", "Capsule", "Torus", "TorusKnot" };
			if( ui::Combo( "Primitive", &currentPrimitive, primitives ) ) {
				switch ( currentPrimitive ) {
					case 0: mModelBatch->replaceVboMesh( gl::VboMesh::create( geom::Sphere().subdivisions( 64 ) ) ); break;
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
			static int currentEnvironment = 0;
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

void PBRTexturingBasicsApp::draw()
{
	// clear window and set matrices
	gl::clear( Color( 1, 0, 0 ) );
	gl::setMatrices( mCamera );
	
	// enable depth testing
	gl::ScopedDepth scopedDepth( true );
	
	// bind the textures
	gl::ScopedTextureBind scopedTexBind0( mRadianceMap, 0 );
	gl::ScopedTextureBind scopedTexBind1( mIrradianceMap, 1 );
	gl::ScopedTextureBind scopedTexBind2( mNormalMap, 2 );
	gl::ScopedTextureBind scopedTexBind3( mRoughnessMap, 3 );
	gl::ScopedTextureBind scopedTexBind4( mMetallicMap, 4 );
	
	auto shader = mModelBatch->getGlslProg();
	shader->uniform( "uRadianceMap", 0 );
	shader->uniform( "uIrradianceMap", 1 );
	shader->uniform( "uNormalMap", 2 );
	shader->uniform( "uRoughnessMap", 3 );
	shader->uniform( "uMetallicMap", 4 );
	shader->uniform( "uRadianceMapSize", (float) mRadianceMap->getWidth() );
	shader->uniform( "uIrradianceMapSize", (float) mIrradianceMap->getWidth() );
	
	// sends the base color, the specular opacity, roughness and metallic to the shader
	shader->uniform( "uBaseColor", mBaseColor );
	shader->uniform( "uSpecular", mSpecular );
	shader->uniform( "uRoughness", mRoughness );
	shader->uniform( "uMetallic", mMetallic );
	
	// sends the tone-mapping uniforms
	shader->uniform( "uExposure", mExposure );
	shader->uniform( "uGamma", mGamma );
	
	// render our test model
	{
		gl::ScopedMatrices scopedMatrices;
		gl::setModelMatrix( glm::rotate( mTime, vec3( 0.123, 0.456, 0.789 ) ) );
		mModelBatch->draw();
	}
	
	// render skybox
	shader = mSkyBoxBatch->getGlslProg();
	shader->uniform( "uExposure", mExposure );
	shader->uniform( "uGamma", mGamma );
	mSkyBoxBatch->draw();
}

CINDER_APP( PBRTexturingBasicsApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )