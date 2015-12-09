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


typedef std::shared_ptr<class CascadedShadows> CascadedShadowsRef;

class CascadedShadows {
public:
	//! construct a CascadedShadowsRef
	static CascadedShadowsRef create();
	//! creates the splits, should be called everytime the camera or the light change
	void update( const CameraPersp &camera, const glm::vec3 &lightDir );
	//! filters the shadowmaps with a gaussian blur
	void filter();
	
	//! sets the shadow maps resolution
	void setResolution( size_t resolution ) { mResolution = resolution; createFramebuffers(); }
	//! sets the over-shadowing constant
	void setShadowingFactor( float factor ) { mExpC = factor; }
	//! sets frustum split constant
	void setSplitLambda( float lambda ) { mSplitLambda = lambda; }
	
	//! returns the shadow maps resolution
	size_t	getResolution() const { return mResolution; }
	//! returns the over-shadowing constant
	float	getShadowingFactor() const { return mExpC; }
	//! returns frustum split constant
	float	getSplitLambda() const { return mSplitLambda; }
	
	//! returns the shadow maps 3d texture
	gl::Texture3dRef		getShadowMap() const { return static_pointer_cast<gl::Texture3d>( mShadowMapArray->getTextureBase( GL_COLOR_ATTACHMENT0 ) ); }
	//! returns the shadow maps framebuffer
	const gl::FboRef&		getShadowMapArray() const { return mShadowMapArray; }
	//! returns the GlslProg used to draw the shadow maps to the screen
	const gl::GlslProgRef&	getDebugProg() const { return mDebugProg; }
	//! returns the cascades split planes
	const vector<vec2>&		getSplitPlanes() const { return mSplitPlanes; }
	//! returns the cascades view matrices
	const vector<mat4>&		getViewMatrices() const { return mViewMatrices; }
	//! returns the cascades projection matrices
	const vector<mat4>&		getProjMatrices() const { return mProjMatrices; }
	//! returns the cascades shadow matrices
	const vector<mat4>&		getShadowMatrices() const { return mShadowMatrices; }
	//! returns the cascades near planes
	const vector<float>&	getNearPlanes() const { return mNearPlanes; }
	//! returns the cascades far planes
	const vector<float>&	getFarPlanes() const { return mFarPlanes; }
	
	CascadedShadows();
	
protected:
	void createFramebuffers();
	
	gl::FboRef			mShadowMapArray;
	gl::FboRef			mBlurFbo;
	gl::GlslProgRef		mDebugProg;
	gl::GlslProgRef		mFilterProg;
	
	size_t				mResolution;
	float				mExpC;
	float				mSplitLambda;
	vector<vec2>		mSplitPlanes;
	vector<mat4>		mViewMatrices;
	vector<mat4>		mProjMatrices;
	vector<mat4>		mShadowMatrices;
	vector<float>		mNearPlanes;
	vector<float>		mFarPlanes;
};

class CascadedShadowMappingApp : public App {
  public:
	CascadedShadowMappingApp();
	void update() override;
	void draw() override;
	void resize() override;
	void userInterface();
	
	
	// Scene Objects
	using Object = std::tuple<gl::BatchRef,gl::BatchRef,AxisAlignedBox>;
	CameraPersp			mCamera;
	CameraUi			mCameraUi;
	vector<Object>		mScene;
	vec3				mLightDir;
	
	// Framebuffer and textures
	CascadedShadowsRef	mCascadedShadows;
	gl::Texture2dRef	mAmbientOcclusion;
	
	// options
	bool				mPolygonOffset, mFiltering, mShowCascades, mShowUi, mShowShadowMaps;
	int					mShadowMapSize;
};


CascadedShadowMappingApp::CascadedShadowMappingApp()
{
	// initialize user interface
	ui::initialize();
	
	// load shader
	auto shader = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
	auto shadowShader = gl::GlslProg::create( loadAsset( "shadowmap.vert" ), loadAsset( "shadowmap.frag" ), loadAsset( "shadowmap.geom" ) );
	
	// parse obj and split into gl::Batch
	auto source = ObjLoader( loadAsset( "terrain.obj" ) );
	for( size_t i = 0; i < source.getNumGroups(); ++i ) {
		auto trimesh = TriMesh( source.groupIndex( i ) );
		mScene.push_back( {
			gl::Batch::create( source, shader ),
			gl::Batch::create( source, shadowShader ),
			trimesh.calcBoundingBox()
		});
	}
	
	// load baked ao texture
	mAmbientOcclusion = gl::Texture2d::create( loadImage( loadAsset( "bakedAO.jpg" ) ) );
	
	// create the cascaded shadow map
	mCascadedShadows = CascadedShadows::create();
	
	// setup camera and camera ui
	mCamera		= CameraPersp( getWindowWidth(), getWindowHeight(), 50.0f, 0.1f, 18.0f ).calcFraming( Sphere( vec3( 0.0f ), 5.0f ) );
	mCameraUi	= CameraUi( &mCamera, getWindow(), -1 );
	
	// initial options
	mLightDir		= normalize( vec3( -1.4f, -0.37f, 0.63f ) );
	mShowShadowMaps = false;
	mShowCascades	= false;
	mPolygonOffset	= true;
	mFiltering		= true;
}
void CascadedShadowMappingApp::resize()
{
	// adapt camera ratio
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void CascadedShadowMappingApp::update()
{
	// recreate cascades from the user point of view
	mCascadedShadows->update( mCamera, mLightDir );
	
	// render the shadowmaps
	{
		auto fbo = mCascadedShadows->getShadowMapArray();
		gl::ScopedFramebuffer scopedFbo( fbo );
		gl::ScopedViewport scopedViewport( vec2( 0.0f ), fbo->getSize() );
		gl::ScopedDepth enableDepth( true );
		gl::ScopedBlend disableBlending( false );
		gl::ScopedFaceCulling scopedCulling( true, GL_BACK );
		
		// polygon offset fixes some really small artifacts at grazing angles
		if( mPolygonOffset ) {
			gl::enable( GL_POLYGON_OFFSET_FILL );
			glPolygonOffset( 2.0f, 2.0f );
		}
		
		gl::clear( Color( 1.0f, 0.0f, 0.0f ) );
		
		for( const auto &obj : mScene ) {
			auto batch	= std::get<1>( obj );
			auto shader = batch->getGlslProg();
			shader->uniform( "uCascadesViewMatrices", mCascadedShadows->getViewMatrices().data(), mCascadedShadows->getViewMatrices().size() );
			shader->uniform( "uCascadesProjMatrices", mCascadedShadows->getProjMatrices().data(), mCascadedShadows->getProjMatrices().size() );
			shader->uniform( "uCascadesNear", mCascadedShadows->getNearPlanes().data(), mCascadedShadows->getNearPlanes().size() );
			shader->uniform( "uCascadesFar", mCascadedShadows->getFarPlanes().data(), mCascadedShadows->getFarPlanes().size() );
			batch->draw();
		}
		
		if( mPolygonOffset )
			gl::disable( GL_POLYGON_OFFSET_FILL );
	}
	
	// filter if needed
	if( mFiltering )
		mCascadedShadows->filter();
	
	userInterface();
}

void CascadedShadowMappingApp::draw()
{
	gl::clear( Color::gray( 0.72f ) );
	
	// render scene
	gl::ScopedDepth enableDepth( true );
	gl::ScopedBlend disableBlending( false );
	gl::ScopedFaceCulling scopedCulling( true, GL_BACK );
	gl::ScopedTextureBind scopedTexBind0( mCascadedShadows->getShadowMap(), 0 );
	gl::ScopedTextureBind scopedTexBind1( mAmbientOcclusion, 1 );
	
	gl::setMatrices( mCamera );
	
	Frustumf frustum( mCamera );
	for( const auto &obj : mScene ) {
		if( frustum.intersects( std::get<2>( obj ) ) ) {
			auto batch	= std::get<0>( obj );
			auto shader = batch->getGlslProg();
			
			vec3 lightDir = normalize( vec3( mCamera.getViewMatrix() * vec4( mLightDir, 0.0f ) ) );
			
			shader->uniform( "uLightDirection", lightDir );
			shader->uniform( "uExpC", mCascadedShadows->getShadowingFactor() );
			shader->uniform( "uShadowMap", 0 );
			shader->uniform( "uAmbientOcclusion", 1 );
			shader->uniform( "uCascadesNear", mCascadedShadows->getNearPlanes().data(), mCascadedShadows->getNearPlanes().size() );
			shader->uniform( "uCascadesFar", mCascadedShadows->getFarPlanes().data(), mCascadedShadows->getFarPlanes().size() );
			shader->uniform( "uCascadesPlanes", mCascadedShadows->getSplitPlanes().data(), mCascadedShadows->getSplitPlanes().size() );
			shader->uniform( "uCascadesMatrices", mCascadedShadows->getShadowMatrices().data(), mCascadedShadows->getShadowMatrices().size() );
			shader->uniform( "uShowCascades", mShowCascades ? 1.0f : 0.0f );
			batch->draw();
		}
	}
	
	// display array of shadow maps
	if( mShowShadowMaps ) {
		gl::disableDepthRead();
		gl::disableDepthWrite();
		gl::setMatricesWindow( getWindowSize() );
		
		auto prog = mCascadedShadows->getDebugProg();
		gl::ScopedGlslProg scopedGlsl( prog );
		gl::ScopedTextureBind texBind( mCascadedShadows->getShadowMap() );
		for( size_t i = 0; i < 4; i++ ) {
			prog->uniform( "uSection", static_cast<int>( i ) );
			gl::drawSolidRect( Rectf( vec2(64*i,0), vec2(64*i,0)+vec2(64) ) );
		}
	}
}

void CascadedShadowMappingApp::userInterface()
{
	ui::ScopedWindow window( "Cascaded Shadow Mapping" );
	
	// Light and Camera options
	if( ui::CollapsingHeader( "Light", nullptr, true, true ) ) {
		ui::DragFloat3( "Direction", &mLightDir[0], 0.01f );
	}
	if( ui::CollapsingHeader( "Camera", nullptr, true, true ) ) {
		float near = mCamera.getNearClip();
		if( ui::DragFloat( "Camera NearClip", &near, 0.1f, 0.0f, mCamera.getFarClip() ) ) mCamera.setNearClip( near );
		float far = mCamera.getFarClip();
		if( ui::DragFloat( "Camera FarClip", &far, 0.1f, mCamera.getNearClip() ) ) mCamera.setFarClip( far );
	}
	
	// cascade options
	if( ui::CollapsingHeader( "Shadow Mapping", nullptr, true, true ) ) {
		ui::Checkbox( "Filtering", &mFiltering );
		ui::Checkbox( "Polygon Offset", &mPolygonOffset );
		float shadowing = mCascadedShadows->getShadowingFactor();
		if( ui::DragFloat( "Shadowing Factor", &shadowing, 1.0f, 0.0f, 1000.0f ) ) mCascadedShadows->setShadowingFactor( shadowing );
		float splitLambda = mCascadedShadows->getSplitLambda();
		if( ui::DragFloat( "SplitLambda", &splitLambda, 0.001f, 0.0f ) ) mCascadedShadows->setSplitLambda( splitLambda );
	}
	
	// debug options
	if( ui::CollapsingHeader( "Debug", nullptr, true, true ) ) {
		ui::Checkbox( "Show Cascades", &mShowCascades );
		ui::Checkbox( "Show ShadowMaps", &mShowShadowMaps );
	}
	
	// update window title
	getWindow()->setTitle( "Cascaded Shadow Mapping | " + to_string( (int) getAverageFps() ) + " fps" );
}

CascadedShadowsRef CascadedShadows::create()
{
	return make_shared<CascadedShadows>();
}

CascadedShadows::CascadedShadows()
: mResolution( 2048 ), mExpC( 120.0f ), mSplitLambda( 0.5f )
{
	// load shaders
	auto format = gl::GlslProg::Format().vertex( loadAsset( "gaussian.vert" ) ).fragment( loadAsset( "gaussian.frag" ) ).geometry( loadAsset( "gaussian.geom" ) ).define( "KERNEL", "KERNEL_7x7_GAUSSIAN" );
	mFilterProg	= gl::GlslProg::create( format );
	mDebugProg	= gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "debugShadowmap.vert" ) ).fragment( loadAsset( "debugShadowmap.frag" ) ) );
	
	// create framebuffers
	createFramebuffers();
}
void CascadedShadows::update( const CameraPersp &camera, const glm::vec3 &lightDir )
{
	// clear vectors
	mViewMatrices.clear();
	mProjMatrices.clear();
	mShadowMatrices.clear();
	mSplitPlanes.clear();
	mNearPlanes.clear();
	mFarPlanes.clear();
	
	// calculate splits
	float near = camera.getNearClip();
	float far = camera.getFarClip();
	for( size_t i = 0; i < 4; ++i ) {
		// find the split planes using GPU Gem 3. Chap 10 "Practical Split Scheme".
		float splitNear = i > 0 ? glm::mix( near + ( static_cast<float>( i ) / 4.0f ) * ( far - near ), near * pow( far / near, static_cast<float>( i ) / 4.0f ), mSplitLambda ) : near;
		float splitFar = i < 4 - 1 ? glm::mix( near + ( static_cast<float>( i + 1 ) / 4.0f ) * ( far - near ), near * pow( far / near, static_cast<float>( i + 1 ) / 4.0f ), mSplitLambda ) : far;
		
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
		mat4 viewMat = glm::lookAt( vec3( splitCentroid ) - lightDir * dist, vec3( splitCentroid ), vec3( 0.0f, 1.0f, 0.0f ) );
		
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
		static const mat4 offsetMat = mat4( vec4( 0.5f, 0.0f, 0.0f, 0.0f ), vec4( 0.0f, 0.5f, 0.0f, 0.0f ), vec4( 0.0f, 0.0f, 0.5f, 0.0f ), vec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
		
		// save matrices and near/far planes
		mViewMatrices.push_back( viewMat );
		mProjMatrices.push_back( projMat );
		mShadowMatrices.push_back( offsetMat * projMat * viewMat );
		mSplitPlanes.push_back( vec2( splitNear, splitFar ) );
		mNearPlanes.push_back( -max.z - nearOffset );
		mFarPlanes.push_back( -min.z + farOffset );
	}
}
void CascadedShadows::filter()
{
	// setup rendering for fullscreen quads
	gl::ScopedMatrices scopedMatrices;
	gl::ScopedViewport scopedViewport( ivec2(0), mBlurFbo->getSize() );
	gl::ScopedDepth scopedDepth( false );
	gl::ScopedGlslProg scopedGlsl( mFilterProg );
	gl::ScopedBlend scopedBlend( false );
	gl::ScopedFramebuffer scopedFbo( mBlurFbo );
	gl::setMatricesWindow( mBlurFbo->getSize() );
	
	gl::clear();
	
	// two pass gaussian blur
	mFilterProg->uniform( "uSampler", 0 );
	mFilterProg->uniform( "uInvSize", vec2( 1.0f ) / vec2( mBlurFbo->getSize() ) );
	
	// horizontal pass
	mFilterProg->uniform( "uDirection", vec2( 1.0f, 0.0f ) );
	gl::ScopedTextureBind scopedTexBind0( mShadowMapArray->getTextureBase( GL_COLOR_ATTACHMENT0 ), 0 );
	gl::drawBuffer( GL_COLOR_ATTACHMENT0 );
	gl::drawSolidRect( mBlurFbo->getBounds() );
	
	// vertical pass
	mFilterProg->uniform( "uDirection", vec2( 0.0f, 1.0f ) );
	gl::ScopedTextureBind scopedTexBind1( mBlurFbo->getTextureBase( GL_COLOR_ATTACHMENT0 ), 0 );
	gl::drawBuffer( GL_COLOR_ATTACHMENT1 );
	gl::drawSolidRect( mBlurFbo->getBounds() );
	
	gl::drawBuffer( GL_COLOR_ATTACHMENT0 );
}

void CascadedShadows::createFramebuffers()
{
	// create a layered framebuffer for the different shadow maps
	auto textureArrayFormat = gl::Texture3d::Format().target( GL_TEXTURE_2D_ARRAY ).internalFormat( GL_R16F ).magFilter( GL_LINEAR ).minFilter( GL_LINEAR ).wrap( GL_CLAMP_TO_EDGE );
	auto textureArray = gl::Texture3d::create( mResolution, mResolution, 4, textureArrayFormat );
	auto textureArrayDepth = gl::Texture3d::create( mResolution, mResolution, 4, gl::Texture3d::Format().target( GL_TEXTURE_2D_ARRAY ).internalFormat( GL_DEPTH_COMPONENT24 ) );
	mShadowMapArray = gl::Fbo::create( mResolution, mResolution, gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, textureArray ).attachment( GL_DEPTH_ATTACHMENT, textureArrayDepth ) );
	
	// create a second layered framebuffer for filtering using the same attachement has the shadowmap framebuffer
	auto blurAtt0 = gl::Texture3d::create( mResolution, mResolution, 4, textureArrayFormat );
	auto blurFormat = gl::Fbo::Format().attachment( GL_COLOR_ATTACHMENT0, blurAtt0 ).attachment( GL_COLOR_ATTACHMENT1, textureArray ).disableDepth();
	mBlurFbo = gl::Fbo::create( mResolution, mResolution, blurFormat );
}


CINDER_APP( CascadedShadowMappingApp, RendererGl( RendererGl::Options().msaa( 4 ) ), []( App::Settings *settings ) {
	settings->setWindowSize( 1280, 800 );
})
