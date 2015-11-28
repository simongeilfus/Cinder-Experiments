#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"

#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PBRBasicsApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void resize() override;
	
	void renderAnnotations();
	
	CameraPersp		mCamera;
	CameraUi		mCameraUi;
	gl::BatchRef	mSphereBatch, mLightBatch;
	vec3			mLightPosition;
	
	int				mGridSize;
	bool			mAnimateLight, mShowUi;
	float			mRoughness, mMetallic, mSpecular;
	Color			mBaseColor, mLightColor;
	float			mLightRadius, mGamma, mExposure, mTime;
	
	Font			mFont;
};

void PBRBasicsApp::setup()
{
	// create a Camera and a Camera ui
	mCamera		= CameraPersp( getWindowWidth(), getWindowHeight(), 50.0f, 1.0f, 100.0f ).calcFraming( Sphere( vec3( 0.0f ), 12.0f ) );
	mCameraUi	= CameraUi( &mCamera, getWindow(), -1 );
	
	// prepare ou rendering objects and shaders
	auto pbrShader	= gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "PBR.vert" ) ).fragment( loadAsset( "PBR.frag" ) ) );
	mSphereBatch	= gl::Batch::create( geom::Sphere().subdivisions( 32 ), pbrShader );
	mLightBatch		= gl::Batch::create( geom::Sphere().subdivisions( 32 ), gl::getStockShader( gl::ShaderDef().color() ) );
	
	// set the initial parameters and setup the ui
	mGridSize			= 6;
	mRoughness			= 1.0f;
	mMetallic			= 1.0f;
	mSpecular			= 1.0f;
	mLightRadius		= 4.0f;
	mLightColor			= Color::white();
	mBaseColor			= Color( 1.0f, 0.0f, 0.0f );
	mTime				= 0.0f;
	mGamma				= 2.2f;
	mExposure			= 10.0f;
	mAnimateLight		= true;
	mShowUi				= false;
	
	// prepare ui and load font
	ui::initialize();
	mFont = Font( "Arial", 12 );
	getWindow()->getSignalKeyDown().connect( [this]( KeyEvent event ) {
		if( event.getCode() == KeyEvent::KEY_SPACE ) mShowUi = !mShowUi;
	} );
}

void PBRBasicsApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void PBRBasicsApp::update()
{
	// animate the light
	if( mAnimateLight ){
		mLightPosition = vec3( cos( mTime * 0.5f ) * 8.0f, 8.0f + sin( mTime * 0.25f ) * 3.5f, sin( mTime * 0.5f ) * 8.0f );
		mTime += 0.025f;
	}
	
	// user interface
	if( mShowUi ) {
		ui::ScopedWindow window( "PBRBasics" );
		if( ui::CollapsingHeader( "Material", nullptr, true, true ) ) {
			ui::DragFloat( "Roughness", &mRoughness, 0.01f, 0.0f, 1.0f );
			ui::DragFloat( "Metallic", &mMetallic, 0.01f, 0.0f, 1.0f );
			ui::DragFloat( "Specular", &mSpecular, 0.01f, 0.0f, 1.0f );
			ui::ColorEdit3( "Color", &mBaseColor[0] );
		}
		if( ui::CollapsingHeader( "Light", nullptr, true, true ) ) {
			ui::Checkbox( "Animation", &mAnimateLight );
			ui::DragFloat( "Radius", &mLightRadius, 0.1f, 0.0f, 20.0f );
			ui::ColorEdit3( "Color###LightColor", &mLightColor[0] );
		}
		if( ui::CollapsingHeader( "Rendering", nullptr, true, true ) ) {
			ui::DragFloat( "Gamma", &mGamma, 0.01f, 0.0f );
			ui::DragFloat( "Exposure", &mExposure, 0.01f, 0.0f );
		}
	}
}

void PBRBasicsApp::draw()
{
	// clear window and set matrices
	gl::clear( Color( 0, 0, 0 ) );
	gl::setMatrices( mCamera );
	
	// enable depth testing
	gl::ScopedDepth scopedDepth( true );
	
	// sends the base color, the specular opacity,
	// the light position, color and radius to the shader
	auto shader = mSphereBatch->getGlslProg();
	shader->uniform( "uLightPosition", mLightPosition );
	shader->uniform( "uLightColor", mLightColor );
	shader->uniform( "uLightRadius", mLightRadius );
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
				float roughness = lmap( (float) z, (float) -mGridSize, (float) mGridSize, 0.05f, 1.0f );
				float metallic	= lmap( (float) x, (float) -mGridSize, (float) mGridSize, 1.0f, 0.0f );
				
				shader->uniform( "uRoughness", pow( roughness * mRoughness, 4.0f ) );
				shader->uniform( "uMetallic", metallic * mMetallic );
				
				gl::setModelMatrix( glm::translate( vec3( x, 0, z ) * 2.25f ) );
				mSphereBatch->draw();
			}
		}
	}
	
	// render the light
	{
		gl::ScopedMatrices scopedMatrices;
		gl::color( mLightColor + Color::white() * 0.5f );
		gl::setModelMatrix( glm::translate( mLightPosition ) * glm::scale( vec3( mLightRadius * 0.15f ) ) );
		mLightBatch->draw();
	}
	
	// display annotations
	renderAnnotations();
}

void PBRBasicsApp::renderAnnotations()
{
	
	// render the metal/roughness axis
	// fade the color when the camera is low
	gl::ScopedBlendAlpha alphaBlending;
	gl::ScopedDepth enableDepth( true );
	vec3 camAngles = glm::eulerAngles( mCamera.getOrientation() );
	float pitch = camAngles.x;
	float pitchFade = ( 1.0f - abs( cos( pitch ) ) ) * 0.5f;
	gl::setMatrices( mCamera );
	gl::color( ColorA( 1.0f, 1.0f, 1.0f, pitchFade ) );
	gl::drawVector( vec3( mGridSize * 2.25f, 0, -mGridSize * 3.0f ) , vec3( -mGridSize * 2.25f, 0, -mGridSize * 3.0f ), 0.8f, 0.2f );
	gl::drawVector( vec3( mGridSize * 3.0f, 0, -mGridSize * 2.25f ) , vec3( mGridSize * 3.0f, 0, mGridSize * 2.25f ), 0.8f, 0.2f );
	
	// render the ui and annotations
	gl::ScopedDepth disableDepth( false );
	gl::setMatricesWindow( getWindowSize() );
	
	// project text positions
	mat4 view			= mCamera.getViewMatrix();
	mat4 proj			= mCamera.getProjectionMatrix();
	vec4 viewport		= vec4(0,0,getWindowSize());
	vec3 center			= glm::project( vec3( mGridSize * 3.0f, 2.0f, -mGridSize * 3.0f ), view, proj, viewport );
	vec3 roughness		= glm::project( vec3( mGridSize * 3.0f + 2.0f, 2.0f, 0 ), view, proj, viewport );
	vec3 roughnessEnd	= glm::project( vec3( mGridSize * 3.0f, 1.0f, mGridSize * 3.0f ), view, proj, viewport );
	vec3 metal			= glm::project( vec3( 0, 2.0f, -mGridSize * 3.0f - 2.0f ), view, proj, viewport );
	vec3 metalEnd		= glm::project( vec3( -mGridSize * 3.0f, 1.0f, -mGridSize * 3.0f ), view, proj, viewport );
	
	gl::drawStringCentered( "0.0", vec2( center.x, getWindowHeight() - center.y ), ColorA( 1.0f, 1.0f, 1.0f, pitchFade ), mFont );
	
	float angle = atan2( ( getWindowHeight() - center.y ) - ( getWindowHeight() - roughnessEnd.y ), center.x - roughnessEnd.x );
	gl::pushModelMatrix();
	gl::translate( vec2( roughness.x, getWindowHeight() - roughness.y ) );
	gl::rotate( angle + ( camAngles.y < 0.0f ? M_PI : 0.0f ) );
	gl::drawStringCentered( "Roughness", vec2( 0.0f ), ColorA( 1.0f, 1.0f, 1.0f, pitchFade ), mFont );
	gl::popModelMatrix();
	gl::pushModelMatrix();
	gl::translate( vec2( roughnessEnd.x, getWindowHeight() - roughnessEnd.y ) );
	gl::rotate( angle + ( camAngles.y < 0.0f ? M_PI : 0.0f ) );
	gl::drawStringCentered( to_string( mRoughness ).substr( 0, 3 ), vec2( 0.0f ), ColorA( 1.0f, 1.0f, 1.0f, pitchFade ), mFont );
	gl::popModelMatrix();
	
	angle = atan2( ( getWindowHeight() - center.y ) - ( getWindowHeight() - metalEnd.y ), center.x - metalEnd.x );
	gl::pushModelMatrix();
	gl::translate( vec2( metal.x, getWindowHeight() - metal.y - 10.0f ) );
	gl::rotate( angle + ( abs( angle ) < M_PI_2 ? 0.0f : M_PI ) );
	gl::drawStringCentered( "Metal", vec2( 0.0f ), ColorA( 1.0f, 1.0f, 1.0f, pitchFade ), mFont );
	gl::popModelMatrix();
	gl::pushModelMatrix();
	gl::translate( vec2( metalEnd.x, getWindowHeight() - metalEnd.y ) );
	gl::rotate( angle + ( abs( angle ) < M_PI_2 ? 0.0f : M_PI ) );
	gl::drawStringCentered( to_string( mMetallic ).substr( 0, 3 ), vec2( 0.0f ), ColorA( 1.0f, 1.0f, 1.0f, pitchFade ), mFont );
	gl::popModelMatrix();
}

CINDER_APP( PBRBasicsApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
