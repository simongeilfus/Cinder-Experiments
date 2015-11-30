#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/ObjLoader.h"

#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/
// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
// https://software.intel.com/en-us/articles/sample-distribution-shadow-maps
// http://blogs.aerys.in/jeanmarc-leroux/2015/01/21/exponential-cascaded-shadow-mapping-with-webgl/
// https://github.com/NVIDIAGameWorks/OpenGLSamples/blob/master/samples/gl4-maxwell/CascadedShadowMapping/CascadedShadowMappingRenderer.cpp

class CascadedShadowMappingApp : public App {
  public:
	CascadedShadowMappingApp();
	void update() override;
	void draw() override;
	
	void createShadowMap();
	void updateCascades( const CameraPersp &camera );
	void renderShadowMap();
	void filterShadowMap();
	void userInterface();
	
	// Scene Objects
	CameraPersp			mCamera;
	CameraUi			mCameraUi;
	
	gl::BatchRef mBatch, mShadowBatch;
	
	vec3	mLightDir;
	
	size_t				mCascadesCount;
	vector<vec2>		mCascadesPlanes;
	vector<mat4>		mCascadesViewMatrices;
	vector<mat4>		mCascadesProjMatrices;
	vector<mat4>		mCascadesMatrices;
	vector<float>		mCascadesNear;
	vector<float>		mCascadesFar;
	
	gl::FboRef			mShadowMapArray;
	gl::FboRef			mBlurFbo;
	gl::GlslProgRef		mShadowMapDebugProg;
	gl::GlslProgRef		mGaussianBlur;
	gl::Texture2dRef	mAmbientOcclusion;
	
	// options
	float				mExpC;
	bool				mPolygonOffset, mFiltering, mMultisampling, mShowErrors, mAnimateLight, mShowCascades, mShowUi;
	int					mMultisamplingSamples;
	int					mShadowMapSize;
};

CascadedShadowMappingApp::CascadedShadowMappingApp()
{
	cout << fs::path( fs::path( __FILE__ ).parent_path().parent_path().parent_path() / "/common" ) << endl;
	
	auto shader = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
	auto shadowShader = gl::GlslProg::create( loadAsset( "shadowmap.vert" ), loadAsset( "shadowmap.frag" ), loadAsset( "shadowmap.geom" ) );
	auto source = ObjLoader( loadAsset( "Terrain2.obj" ) );
	mShadowBatch = gl::Batch::create( ObjLoader( loadAsset( "Terrain2.obj" ) ), shadowShader );
	mBatch = gl::Batch::create( source, shader );
	
	mAmbientOcclusion = gl::Texture2d::create( loadImage( loadAsset( "Landscape_1Ambient_Occlusion.png" ) ) );
	
	// load the gaussian blur shader
	mGaussianBlur = gl::GlslProg::create( gl::GlslProg::Format()
										 .vertex( loadAsset( "gaussian.vert" ) )
										 .fragment( loadAsset( "gaussian.frag" ) )
										 .geometry( loadAsset( "gaussian.geom" ) )
										 .define( "KERNEL", "KERNEL_7x7_GAUSSIAN" ) );
	
	getWindow()->getSignalKeyDown().connect( [this]( KeyEvent event ) {
		try {
			auto shader = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
			mBatch->replaceGlslProg( shader );
		}
		catch( const ci::Exception &exc ) {
			cout << exc.what() << endl;
		}
		try {
			auto shadowShader = gl::GlslProg::create( loadAsset( "shadowmap.vert" ), loadAsset( "shadowmap.frag" ), loadAsset( "shadowmap.geom" ) );
			mShadowBatch->replaceGlslProg( shadowShader );
		}
		catch( const ci::Exception &exc ) {
			cout << exc.what() << endl;
		}
	} );
	
	
	mShadowMapDebugProg = gl::GlslProg::create( loadAsset( "debugShadowmap.vert" ), loadAsset( "debugShadowmap.frag" ) );
	
	ui::initialize();
	
	// setup camera and camera ui
	mCamera		= CameraPersp( getWindowWidth(), getWindowHeight(), 50.0f, 1.0f, 50.0f ).calcFraming( Sphere( vec3( 0.0f ), 5.0f ) );
	mCameraUi	= CameraUi( &mCamera, getWindow(), -1 );
	
	// initialize light / splits values
	mCascadesCount = 4;
	mLightDir = normalize( vec3( -1.0f, -0.7f, 1.0f ) );
	
	setWindowSize( 1024, 1024 );
	
	mExpC		= 140.0f;
	
	mShowCascades = false;
}

void CascadedShadowMappingApp::update()
{
	
	//mLightDir = normalize( vec3( cos( getElapsedSeconds() * 0.15 ), -0.4f, sin( getElapsedSeconds() * 0.15 ) ) );
	updateCascades( mCamera );
	renderShadowMap();
	filterShadowMap();
	
	getWindow()->setTitle( "Cascaded Shadow Mapping | " + to_string( (int) getAverageFps() ) + " fps" );
}

void CascadedShadowMappingApp::draw()
{
	gl::clear( Color::gray( 0.72f ) );
	
	gl::ScopedDepth enableDepth( true );
	gl::ScopedBlend disableBlending( false );
	gl::ScopedFaceCulling scopedCulling( true, GL_BACK );
	gl::ScopedTextureBind scopedTexBind0( mShadowMapArray->getTextureBase( GL_COLOR_ATTACHMENT0 ), 0 );
	gl::ScopedTextureBind scopedTexBind1( mAmbientOcclusion, 1 );
	
	ui::DragFloat( "uExpC", &mExpC, 1.0f, 0.0f, 1000.0f );
	static bool useLightCam = false;
	ui::Checkbox( "useLightCam", &useLightCam );
	ui::Checkbox( "ShowCascades", &mShowCascades );
	
	gl::setMatrices( mCamera );
	
	static int splits = mCascadesCount;
	if( ui::SliderInt( "Splits", &splits, 1, 8 ) ) mCascadesCount = splits;
	
	gl::color( Color( 1.0f, 1.0f, 1.0f ) );
	auto shader = mBatch->getGlslProg();
	
	vec3 lightDir = normalize( vec3( mCamera.getViewMatrix() * vec4( mLightDir, 0.0f ) ) );
	mat4 offsetMat = mat4( vec4( 0.5f, 0.0f, 0.0f, 0.0f ), vec4( 0.0f, 0.5f, 0.0f, 0.0f ), vec4( 0.0f, 0.0f, 0.5f, 0.0f ), vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
	
	shader->uniform( "uLightDirection", lightDir );
	shader->uniform( "uExpC", mExpC );
	shader->uniform( "uShadowMap", 0 );
	shader->uniform( "uAmbientOcclusion", 1 );
	shader->uniform( "uOffsetMat", offsetMat );
	shader->uniform( "uCascadesNear", mCascadesNear.data(), mCascadesNear.size() );
	shader->uniform( "uCascadesFar", mCascadesFar.data(), mCascadesFar.size() );
	shader->uniform( "uCascadesPlanes", mCascadesPlanes.data(), mCascadesPlanes.size() );
	shader->uniform( "uCascadesMatrices", mCascadesMatrices.data(), mCascadesMatrices.size() );
	shader->uniform( "uShowCascades", mShowCascades ? 1.0f : 0.0f );
	mBatch->draw();
	
	
	gl::disableDepthRead();
	gl::disableDepthWrite();
	gl::setMatricesWindow( getWindowSize() );
	gl::ScopedGlslProg scopedGlsl( mShadowMapDebugProg );
	gl::ScopedTextureBind texBind( mShadowMapArray->getTextureBase( GL_COLOR_ATTACHMENT0 ) );
	mShadowMapDebugProg->uniform( "uCascadesPlanes", &mCascadesPlanes[0], mCascadesPlanes.size() );
	for( size_t i = 0; i < 4; i++ ) {
		mShadowMapDebugProg->uniform( "uSection", static_cast<int>( i ) );
		gl::drawSolidRect( Rectf( vec2(64*i,0), vec2(64*i,0)+vec2(64) ) );
	}
}

void CascadedShadowMappingApp::renderShadowMap()
{
	
	gl::ScopedFramebuffer scopedFbo( mShadowMapArray );
	gl::ScopedViewport scopedViewport( vec2( 0.0f ), mShadowMapArray->getSize() );
	gl::ScopedDepth enableDepth( true );
	gl::ScopedBlend disableBlending( false );
	gl::ScopedFaceCulling scopedCulling( true, GL_BACK );
	bool mPolygonOffset = true;
	// polygon offset fixes some really small artifacts at grazing angles
	if( mPolygonOffset ) {
		gl::enable( GL_POLYGON_OFFSET_FILL );
		glPolygonOffset( 2.0f, 2.0f );
	}
	
	gl::clear( Color( 0.0f, 0.0f, 0.0f ) );
	
	auto shader = mShadowBatch->getGlslProg();
	shader->uniform( "uCascadesViewMatrices", mCascadesViewMatrices.data(), mCascadesViewMatrices.size() );
	shader->uniform( "uCascadesProjMatrices", mCascadesProjMatrices.data(), mCascadesProjMatrices.size() );
	shader->uniform( "uCascadesNear", mCascadesNear.data(), mCascadesNear.size() );
	shader->uniform( "uCascadesFar", mCascadesFar.data(), mCascadesFar.size() );
	//shader->uniform( "uExpC", mExpC );
	//for( int i = 0; i < mCascadesCount; i++ ){
		//cout << mCascadesNear[i] << endl;
		//shader->uniform( "uLayer", i );
		mShadowBatch->draw();
	//}
	
	if( mPolygonOffset ) gl::disable( GL_POLYGON_OFFSET_FILL );
}

void CascadedShadowMappingApp::createShadowMap()
{
	int mShadowMapSize = 1024;
	auto textureArrayFormat = gl::Texture3d::Format().target( GL_TEXTURE_2D_ARRAY ).internalFormat( GL_R32F ).magFilter( GL_LINEAR ).minFilter( GL_LINEAR ).wrap( GL_CLAMP_TO_EDGE );
	auto textureArray = gl::Texture3d::create( mShadowMapSize, mShadowMapSize, mCascadesCount, textureArrayFormat );
	auto textureArrayDepth = gl::Texture3d::create( mShadowMapSize, mShadowMapSize, mCascadesCount, gl::Texture3d::Format().target( GL_TEXTURE_2D_ARRAY ).internalFormat( GL_DEPTH_COMPONENT24 ) );
	mShadowMapArray = gl::Fbo::create( mShadowMapSize, mShadowMapSize, gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, textureArray ).attachment( GL_DEPTH_ATTACHMENT, textureArrayDepth ) );
	
	mBlurFbo = gl::Fbo::create( mShadowMapSize, mShadowMapSize, gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, gl::Texture3d::create( mShadowMapSize, mShadowMapSize, mCascadesCount, textureArrayFormat ) ).attachment( GL_COLOR_ATTACHMENT1, textureArray ).disableDepth() );
}

void CascadedShadowMappingApp::updateCascades( const CameraPersp &camera )
{
	// create or re-size shadowmaps when needed
	if( !mShadowMapArray || dynamic_pointer_cast<gl::Texture3d>( mShadowMapArray->getTextureBase( GL_COLOR_ATTACHMENT0 ) )->getDepth() != mCascadesCount ) {
		createShadowMap();
	}
	
	// clear vectors
	mCascadesViewMatrices.clear();
	mCascadesProjMatrices.clear();
	mCascadesMatrices.clear();
	mCascadesPlanes.clear();
	mCascadesNear.clear();
	mCascadesFar.clear();
	
	// calculate splits
	static float lambda	= 0.8f;
	ui::DragFloat( "lambda", &lambda, 0.001f, 0.0f );
	
	static float cascadesNear = 0.01f;
	static float cascadesFar = 35.0f;
	//cascadesFar = glm::length( mCamera.getEyePoint() ) * 2;
	ui::DragFloat( "cascadesNear", &cascadesNear, 0.1f, 0.01f, cascadesFar );
	ui::DragFloat( "cascadesFar", &cascadesFar, 0.1f, cascadesNear );
	
	float near		= cascadesNear;//camera.getNearClip();
	float far		= cascadesFar;
	for( size_t i = 0; i < mCascadesCount; ++i ) {
		// find the split planes using GPU Gem 3. Chap 10 "Practical Split Scheme".
		float splitNear = i > 0 ? glm::mix( near + ( static_cast<float>( i ) / static_cast<float>( mCascadesCount ) ) * ( far - near ), near * pow( far / near, static_cast<float>( i ) / static_cast<float>( mCascadesCount ) ), lambda ) : near;
		float splitFar = i < mCascadesCount - 1 ? glm::mix( near + ( static_cast<float>( i + 1 ) / static_cast<float>( mCascadesCount ) ) * ( far - near ), near * pow( far / near, static_cast<float>( i + 1 ) / static_cast<float>( mCascadesCount ) ), lambda ) : far;
		
		// create a camera for this split
		CameraPersp splitCamera( camera );
		splitCamera.setNearClip( splitNear );
		splitCamera.setFarClip( splitFar );
		
		// extract the split frutum vertices
		vec3 ntl, ntr, nbl, nbr, ftl, ftr, fbl, fbr;
		splitCamera.getNearClipCoordinates( &ntl, &ntr, &nbl, &nbr );
		splitCamera.getFarClipCoordinates( &ftl, &ftr, &fbl, &fbr );
		vec4 splitVertices[8] = { vec4( ntl, 1.0f ), vec4( ntr, 1.0f ), vec4( nbl, 1.0f ), vec4( nbr, 1.0f ), vec4( ftl, 1.0f ), vec4( ftr, 1.0f ), vec4( fbl, 1.0f ), vec4( fbr, 1.0f ) };
		
		// find the split centroid so we can construct a view matrix
		vec4 splitCentroid( 0.0f );
		for( size_t i = 0; i < 8; ++i ) {
			splitCentroid += splitVertices[i];
		}
		splitCentroid /= 8.0f;
		
		// construct the view matrix
		float dist = glm::max( splitFar - splitNear, glm::distance( ftl, ftr ) );
		mat4 viewMat = glm::lookAt( vec3( splitCentroid ) - mLightDir * dist, vec3( splitCentroid ), vec3( 0.0f, 1.0f, 0.0f ) );
		
		// transform split vertices to the light view space
		vec4 splitVerticesLS[8];
		for( size_t i = 0; i < 8; ++i ) {
			splitVerticesLS[i] = viewMat * splitVertices[i];
		}
		
		// find the frustum bounding box in viewspace
		vec4 min = splitVerticesLS[0];
		vec4 max = splitVerticesLS[0];
		for( size_t i = 1; i < 8; ++i ) {
			min = glm::min( min, splitVerticesLS[i] );
			max = glm::max( max, splitVerticesLS[i] );
		}
		
		// and create an orthogonal projection matrix with the corners
		float nearOffset = 10.0f;
		float farOffset = 20.0f;
		mat4 projMat = glm::ortho( min.x, max.x, min.y, max.y, -max.z - nearOffset, -min.z + farOffset );
		
		// save matrices and near/far planes
		mCascadesViewMatrices.push_back( viewMat );
		mCascadesProjMatrices.push_back( projMat );
		mCascadesMatrices.push_back( projMat * viewMat );
		mCascadesPlanes.push_back( vec2( splitNear, splitFar ) );
		mCascadesNear.push_back( -max.z - nearOffset );
		mCascadesFar.push_back( -min.z + farOffset );
	}
}
void CascadedShadowMappingApp::filterShadowMap()
{
	// setup rendering for fullscreen quads
	gl::ScopedMatrices scopedMatrices;
	gl::ScopedViewport scopedViewport( ivec2(0), mBlurFbo->getSize() );
	gl::ScopedDepth scopedDepth( false );
	gl::ScopedGlslProg scopedGlsl( mGaussianBlur );
	gl::ScopedBlend scopedBlend( false );
	gl::ScopedFramebuffer scopedFbo( mBlurFbo );
	gl::setMatricesWindow( mBlurFbo->getSize() );
	
	gl::clear();
	
	// two pass gaussian blur
	mGaussianBlur->uniform( "uSampler", 0 );
	mGaussianBlur->uniform( "uInvSize", vec2( 1.0f ) / vec2( mBlurFbo->getSize() ) );
	
	// horizontal pass
	mGaussianBlur->uniform( "uDirection", vec2( 1.0f, 0.0f ) );
	gl::ScopedTextureBind scopedTexBind0( mShadowMapArray->getTextureBase( GL_COLOR_ATTACHMENT0 ), 0 );
	gl::drawBuffer( GL_COLOR_ATTACHMENT0 );
	gl::drawSolidRect( mBlurFbo->getBounds() );
	
	// vertical pass
	mGaussianBlur->uniform( "uDirection", vec2( 0.0f, 1.0f ) );
	gl::ScopedTextureBind scopedTexBind1( mBlurFbo->getTextureBase( GL_COLOR_ATTACHMENT0 ), 0 );
	gl::drawBuffer( GL_COLOR_ATTACHMENT1 );
	gl::drawSolidRect( mBlurFbo->getBounds() );
	
	gl::drawBuffer( GL_COLOR_ATTACHMENT0 );
}
void CascadedShadowMappingApp::userInterface()
{
	ui::ScopedWindow window( "Cascaded Shadow Mapping" );
	
	float far = mCamera.getFarClip();
	if( ui::DragFloat( "FarClip", &far, 0.1f, mCamera.getNearClip() ) ) {
		mCamera.setFarClip( far );
	}
}
CINDER_APP( CascadedShadowMappingApp, RendererGl( RendererGl::Options().msaa( 4 ) ) )
