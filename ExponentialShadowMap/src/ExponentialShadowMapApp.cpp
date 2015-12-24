#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"

#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#if defined( CINDER_MSW )
	#define M_PI_2 M_PI / 2.0
	#define M_PI_4 M_PI / 4.0
#endif

class ExponentialShadowMapApp : public App {
public:
	ExponentialShadowMapApp();
	void update() override;
	void draw() override;
	
	void createScene();
	void createShadowMap();
	void renderShadowMap();
	void filterShadowMap();
	void userInterface();
	
	// light / shadows
	vec3				mLightPos;
	CameraPersp			mLightCamera;
	gl::FboRef			mLightFbo;
	gl::FboRef			mBlurFbo;
	gl::GlslProgRef		mGaussianBlur;
	
	// Scene Objects
	CameraPersp			mCamera;
	CameraUi			mCameraUi;
	
	using Material = tuple<ci::Color,float,float>;
	using Object = tuple<gl::BatchRef,gl::BatchRef,Material>;
	vector<Object>		mSceneObjects;
	
	// options
	float				mExpC, mErrorEPS;
	bool				mPolygonOffset, mFiltering, mMultisampling, mShowErrors, mAnimateLight, mShowUi;
	int					mMultisamplingSamples;
	int					mShadowMapSize;

};


ExponentialShadowMapApp::ExponentialShadowMapApp()
{
	// setup camera and camera ui
	mCamera		= CameraPersp( getWindowWidth(), getWindowHeight(), 50.0f, 0.1f, 200.0f ).calcFraming( Sphere( vec3( 0.0f ), 5.0f ) );
	mCameraUi	= CameraUi( &mCamera, getWindow(), -1 );
	
	// load the gaussian blur shader
	mGaussianBlur = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "gaussian.vert" ) ).fragment( loadAsset( "gaussian.frag" ) ).define( "KERNEL", "KERNEL_7x7_GAUSSIAN" ) );
	
	// setup a small test scene
	createScene();
	
	// prepare ui
	ui::initialize();
	getWindow()->getSignalKeyDown().connect( [this]( KeyEvent event ) {
		if( event.getCode() == KeyEvent::KEY_SPACE ) mShowUi = !mShowUi;
	} );
	
	// set initial options
	mShadowMapSize			= 2048;
	mExpC					= 110.0f;
	mPolygonOffset			= true;
	mFiltering				= false;
	mMultisampling			= true;
	mMultisamplingSamples	= 4;
	mErrorEPS				= 0.1f;
	mShowErrors				= false;
	mAnimateLight			= true;
	mShowUi					= false;
	
	// create light camera, textures and fbos
	mLightCamera = CameraPersp( 1024, 1024, 80.0f, 0.1f, 18.0f );
	createShadowMap();
}

void ExponentialShadowMapApp::createScene()
{
	// create some geom::Sources
	vector<geom::SourceMods> sources = {
		// add a few basic primitives
		geom::Sphere().radius( 0.5f ).subdivisions( 64 ) >> geom::Translate( vec3( 0.0f, 0.5f, 0.0f ) ),
		geom::Cube() >> geom::Translate( vec3( -2.0f, 0.5f, 0.0f ) ),
		geom::Teapot() >> geom::Translate( vec3( 2.0f, 0.0f, 0.0f ) ),
		geom::Sphere() >> geom::Scale( vec3( 0.08f, 0.01f, 0.05f ) ) >> geom::Translate( 2.93f, 0.74f, 0.0f ), // Teapot cork!!
		geom::Plane().size( vec2( 0.5f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, 2.0f ) ),
		geom::Capsule() >> geom::Scale( vec3( 0.25f ) ) >> geom::Translate( vec3( -2.0f, 0.5f, 2.0f ) ),
		geom::Icosahedron() >> geom::Scale( vec3( 0.25f ) ) >> geom::Translate( vec3( 2.0f, 0.5f, 2.0f ) ),
		// and some more difficult cases
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ), // Wire sphere
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Rotate( M_PI_2, vec3( 1.0f, 0.0f, 0.0f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ),
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Rotate( M_PI_4, vec3( 1.0f, 0.0f, 0.0f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ),
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Rotate( -M_PI_4, vec3( 1.0f, 0.0f, 0.0f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ),
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Rotate( M_PI_2, vec3( 0.0f, 0.0f, 1.0f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ),
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Rotate( M_PI_4, vec3( 0.0f, 0.0f, 1.0f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ),
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Rotate( -M_PI_4, vec3( 0.0f, 0.0f, 1.0f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ),
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Rotate( M_PI_2, vec3( 1.0f, 0.0f, 0.0f ) ) >> geom::Rotate( M_PI_4, vec3( 0.0f, 1.0f, 0.0f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ),
		geom::Torus().ratio( 0.01f ).subdivisionsAxis( 32 ) >> geom::Scale( vec3( 0.5f ) ) >> geom::Rotate( -M_PI_2, vec3( 1.0f, 0.0f, 1.0f ) ) >> geom::Translate( vec3( 0.0f, 0.5f, -2.0f ) ),
		geom::Helix().height( 4.0f ).coils( 8 ).ratio( 0.05f ) >> geom::Scale( vec3( 0.25f ) ) >> geom::Translate( vec3( -2.0f, 0.0f, -2.0f ) ),
		geom::TorusKnot().scale( vec3( 0.25f ) ).radius( 0.05f ) >> geom::Translate( vec3( 2.0f, 0.5f, -2.0f ) ),
		// and a floor
		geom::Plane().size( vec2( 8.0f ) )
	};
	// corresponding materials
	vector<Material> materials = {
		make_tuple( Color( 1.0f, 1.0f, 1.0f ), 0.0005f, 0.0f ),		// Shinny Plastic Sphere
		make_tuple( Color( 1.0f, 1.0f, 1.0f ), 0.25f, 0.0f ),		// White Cube
		make_tuple( Color( 1.0f, 0.766f, 0.336f ), 0.0005f, 0.99f ),// Golden Teapot
		make_tuple( Color( 1.0f, 0.766f, 0.336f ), 0.0005f, 0.99f ),// Golden Teapot Cork!
		make_tuple( Color( 1.0f, 1.0f, 1.0f ), 0.0005f, 0.0f ),		// Shinny Plastic Plane
		make_tuple( Color( 0.672, 0.637, 0.585 ), 0.005f, 0.99f ),	// Platinium Capsule
		make_tuple( Color( 1.0f, 1.0f, 1.0f ), 0.0005f, 0.0f ),		// Plastic Icosahedron
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),// Aluminium Wire sphere
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),
		make_tuple( Color( 0.913f, 0.921f, 0.925f ), 0.005f, 0.98f ),
		make_tuple( Color( 0.955f, 0.637f, 0.538f ), 0.0025f, 0.98f ),// Copper Helix
		make_tuple( Color( 0.972f, 0.960f, 0.915f ), 0.0005f, 0.98f ),// Silver Torus Knot
		make_tuple( Color( 1.0f, 1.0f, 1.0f ), 0.5f, 0.0f )			// Rough Floor
	};
	
	// for each source create gl::Batch for regular rendering and another gl::Batch for rendering the shadow map
	auto shader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "shader.vert" ) ).fragment( loadAsset( "shader.frag" ) ) );
	auto shadowMapShader = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "shadowmap.vert" ) ).fragment( loadAsset( "shadowmap.frag" ) ) );
	for( size_t i = 0; i < sources.size(); ++i ) {
		mSceneObjects.push_back( make_tuple(
			gl::Batch::create( sources[i], shader ),
			gl::Batch::create( sources[i], shadowMapShader ),
			materials[i]
		) );
	}
}

void ExponentialShadowMapApp::update()
{
	// update the ui
	if( mShowUi ) userInterface();
	
	// animate the light
	if( mAnimateLight ) {
		float radius = 4.0f + abs( sin( getElapsedSeconds() * 0.1f ) ) * 4.0f;
		mLightCamera.lookAt( vec3( cos( getElapsedSeconds() * 0.5f ) * radius, 8.0f - radius * 0.5f, sin( getElapsedSeconds() * 0.5f ) * radius ), vec3( 0.0f ) );
	}
	
	getWindow()->setTitle( "Exponential Shadow Mapping | " + to_string( (int) getAverageFps() ) + " fps" );
}

void ExponentialShadowMapApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	// enable depth testing and disable blending
	gl::ScopedDepth scopedDepth( true );
	gl::ScopedBlend disableBlend( false );
	
	// render shadowmap
	renderShadowMap();
	// filter shadowmap
	if( mFiltering ) filterShadowMap();
	
	// render scene
	gl::ScopedMatrices scopedMatrices;
	gl::ScopedFaceCulling scopedCulling( true, GL_BACK );
	gl::ScopedTextureBind scopedTexBind0( mLightFbo->getColorTexture(), 0 );
	gl::setMatrices( mCamera );
	
	// calculate shader uniforms
	float near = mLightCamera.getNearClip();
	float far = mLightCamera.getFarClip();
	float linearDepthScale = 1.0f / ( far - near );
	vec3 lightPos = vec3( mCamera.getViewMatrix() * vec4( mLightCamera.getEyePoint(), 1.0f ) );
	vec3 lightDir = normalize( vec3( mCamera.getViewMatrix() * vec4( mLightCamera.getViewDirection(), 0.0f ) ) );
	mat4 offsetMat = mat4( vec4( 0.5f, 0.0f, 0.0f, 0.0f ), vec4( 0.0f, 0.5f, 0.0f, 0.0f ), vec4( 0.0f, 0.0f, 0.5f, 0.0f ), vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	mat4 lightMat = offsetMat * mLightCamera.getProjectionMatrix() * mLightCamera.getViewMatrix();
	
	// render each objects
	for( auto obj : mSceneObjects ) {
		auto batch = std::get<0>(obj);
		auto material = std::get<2>(obj);
		auto shader = batch->getGlslProg();
		shader->uniform( "uLightPosition", lightPos );
		shader->uniform( "uLightDirection", lightDir );
		shader->uniform( "uLightMatrix", lightMat );
		shader->uniform( "uExpC", mExpC );
		shader->uniform( "uLinearDepthScale", linearDepthScale );
		shader->uniform( "uShadowMap", 0 );
		shader->uniform( "uErrorEPS", mErrorEPS );
		shader->uniform( "uShowErrors", (float) mShowErrors );
		shader->uniform( "uBaseColor", std::get<0>( material ) );
		shader->uniform( "uRoughness", std::get<1>( material ) );
		shader->uniform( "uMetallic", std::get<2>( material ) );
		shader->uniform( "uSpecular", 1.0f );
		batch->draw();
	}
}

void ExponentialShadowMapApp::createShadowMap()
{
	auto shadowMapFormat = gl::Texture2d::Format().internalFormat( GL_R32F ).magFilter( GL_LINEAR ).minFilter( GL_LINEAR ).wrap( GL_CLAMP_TO_EDGE );
	auto shadowMap = gl::Texture2d::create( mShadowMapSize, mShadowMapSize, shadowMapFormat );
	auto shadowFboFormat = gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, shadowMap );
	if( mMultisampling ) shadowFboFormat = shadowFboFormat.samples( mMultisamplingSamples );
	mLightFbo = gl::Fbo::create( mShadowMapSize, mShadowMapSize, shadowFboFormat );
	mBlurFbo = gl::Fbo::create( mShadowMapSize, mShadowMapSize, gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( mShadowMapSize, mShadowMapSize, shadowMapFormat ) ).attachment( GL_COLOR_ATTACHMENT1, shadowMap ) );
}

void ExponentialShadowMapApp::renderShadowMap()
{
	// save the matrices and viewport and bind the light fbo
	gl::ScopedMatrices scopedMatrices;
	gl::ScopedViewport scopedViewport( ivec2(0), mLightFbo->getSize() );
	gl::ScopedFramebuffer scopedFbo( mLightFbo );
	gl::ScopedFaceCulling scopedCulling( true, GL_BACK );
	
	// set the light matrice
	gl::setMatrices( mLightCamera );
	
	// clear with 1.0 as 1.0 is the max depth
	gl::clear( Color( 1.0f, 0.0f, 0.0f ) );
	
	// polygon offset fixes some really small artifacts at grazing angles
	if( mPolygonOffset ) {
		gl::enable( GL_POLYGON_OFFSET_FILL );
		glPolygonOffset( 2.0f, 2.0f );
	}
	
	// calculate shader uniforms
	float near = mLightCamera.getNearClip();
	float far = mLightCamera.getFarClip();
	float linearDepthScale = 1.0f / ( far - near );
	
	// render each objects
	for( auto obj : mSceneObjects ) {
		auto batch = std::get<1>(obj);
		auto shader = batch->getGlslProg();
		shader->uniform( "uLinearDepthScale", linearDepthScale );
		shader->uniform( "uExpC", mExpC );
		batch->draw();
	}
	
	if( mPolygonOffset ) gl::disable( GL_POLYGON_OFFSET_FILL );
}
void ExponentialShadowMapApp::filterShadowMap()
{
	// setup rendering so we can render fullscreen quads
	gl::ScopedMatrices scopedMatrices;
	gl::ScopedViewport scopedViewport( ivec2(0), mBlurFbo->getSize() );
	gl::ScopedDepth scopedDepth( false );
	gl::ScopedGlslProg scopedGlsl( mGaussianBlur );
	gl::ScopedFramebuffer scopedFbo( mBlurFbo );
	gl::setMatricesWindow( mBlurFbo->getSize() );
	
	// two pass gaussian blur
	mGaussianBlur->uniform( "uSampler", 0 );
	mGaussianBlur->uniform( "uInvSize", vec2( 1.0f ) / vec2( mBlurFbo->getSize() ) );
	
	// horizontal pass
	mGaussianBlur->uniform( "uDirection", vec2( 1.0f, 0.0f ) );
	gl::ScopedTextureBind scopedTexBind0( mLightFbo->getColorTexture(), 0 );
	gl::drawBuffer( GL_COLOR_ATTACHMENT0 );
	gl::drawSolidRect( mBlurFbo->getBounds() );
	
	// vertical pass
	mGaussianBlur->uniform( "uDirection", vec2( 0.0f, 1.0f ) );
	gl::ScopedTextureBind scopedTexBind1( mBlurFbo->getTexture2d( GL_COLOR_ATTACHMENT0 ), 0 );
	gl::drawBuffer( GL_COLOR_ATTACHMENT1 );
	gl::drawSolidRect( mBlurFbo->getBounds() );
	
	gl::drawBuffer( GL_COLOR_ATTACHMENT0 );
}

void ExponentialShadowMapApp::userInterface()
{
	ui::ScopedWindow window( "Exponential Shadow Mapping" );
	ui::Checkbox( "Animate Light", &mAnimateLight );
	
	static int res = 2;
	const static vector<string> resolutions = { "512", "1024", "2048", "4096" };
	if( ui::Combo( "ShadowMap Resolution", &res, resolutions ) ){
		mShadowMapSize = stoi( resolutions[res] );
		if( mShadowMapSize > 2048 ) mMultisampling = false;
		createShadowMap();
	}
	
	ui::Checkbox( "Filtering", &mFiltering );
	if( mFiltering ) {
		ui::ScopedChild child( "Filtering Options", vec2(0,35), true );
		static int kernel = 1;
		const static vector<string> kernels = { "KERNEL_3x3_GAUSSIAN", "KERNEL_7x7_GAUSSIAN", "KERNEL_11x11_GAUSSIAN", "KERNEL_15x15_GAUSSIAN" };
		if( ui::Combo( "KernelSize", &kernel, kernels ) ){
			mGaussianBlur = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "gaussian.vert" ) ).fragment( loadAsset( "gaussian.frag" ) ).define( "KERNEL", kernels[ kernel ] ) );
		}
	}
	if( ui::Checkbox( "Multisampling", &mMultisampling ) ) createShadowMap();
	if( mMultisampling ) {
		ui::ScopedChild child( "Multisampling Options", vec2(0,35), true );
		if( ui::InputInt( "Samples", &mMultisamplingSamples ) ) {
			mMultisamplingSamples = glm::clamp( mMultisamplingSamples, 0, gl::Fbo::getMaxSamples() );
			createShadowMap();
		}
	}
	ui::Checkbox( "PolygonOffset", &mPolygonOffset );
	ui::DragFloat( "C", &mExpC, 1.0f, 0.0f, 1000.0f, "%.3f" );
	//ui::DragFloat( "ErrorEPS", &mErrorEPS, 0.001f, 0.0f, 1.0f, "%.3f" );
	//ui::Checkbox( "ShowErrors", &mShowErrors );
}

CINDER_APP( ExponentialShadowMapApp, RendererGl( RendererGl::Options().msaa( 4 ) ) )
