#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Watchdog.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ColorGradingApp : public App {
public:
	ColorGradingApp();
	~ColorGradingApp();
	void draw() override;
	void keyDown( KeyEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	
	//! Loads the color grading lookup table content from a file
	void readLookupTable( const ci::DataSourceRef &lutImage, const ci::ivec3 &lutSize = ci::ivec3( 32 ) );
	//! Exports the color grading lookup table to a file
	void writeLookupTable( const ci::DataTargetRef &lutImage, const ci::ivec3 &lutSize = ci::ivec3( 32 ), const ci::ImageSourceRef &sourceImage = ci::ImageSourceRef(), bool tryToOpenInPhotoshop = true );
	//! Creates a base color grading lookup table
	ci::Surface8u createLut( const ci::ivec3 &size );
	
	
	gl::Texture2dRef	mSourceTexture;
	gl::Texture3dRef	mColorGradingLut;
	gl::GlslProgRef		mColorGradingProg;
	float				mDiagonal, mDiagonalTarget;
};

ColorGradingApp::ColorGradingApp()
: mDiagonal( 1.0f ), mDiagonalTarget( 1.0f )
{
	// load the source image and the glsl prog
	mSourceTexture		= gl::Texture2d::create( loadImage( loadAsset( "iceland.jpg" ) ) );
	mColorGradingProg	= gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "ColorGrading.vert" ) ).fragment( loadAsset( "ColorGrading.frag" ) ) );
	
	// create the initial lookup table
	readLookupTable( loadAsset( "colorGrading.png" ) );
}
ColorGradingApp::~ColorGradingApp()
{
	// remove the temporary color lut... this is just a sample
	auto newGradingLut = getAssetPath( "tempColorGrading.png" );
	if( !newGradingLut.empty() && fs::exists( newGradingLut ) ){
		fs::remove( newGradingLut );
	}
}

void ColorGradingApp::draw()
{
	// interpolate diagonal value
	mDiagonal += ( mDiagonalTarget - mDiagonal ) * 0.1f;
	
	// clear the screen
	gl::clear( Color( 0, 0, 0 ) );
	
	// render a fullscreen quad with the color grading program
	gl::ScopedGlslProg scopedShader( mColorGradingProg );
	gl::ScopedTextureBind scopedTexBind0( mSourceTexture, 0 );
	gl::ScopedTextureBind scopedTexBind1( mColorGradingLut, 1 );
	mColorGradingProg->uniform( "uSource", 0 );
	mColorGradingProg->uniform( "uColorGradingLUT", 1 );
	mColorGradingProg->uniform( "uDiagonal", mDiagonal );
	
	gl::drawSolidRect( getWindowBounds() );

#if defined( CINDER_COCOA )
	gl::ScopedBlendAlpha alphaBlending;
	gl::drawString( "Press 'e' to edit the color grading lut", vec2( 15 ) );
#endif
}

void ColorGradingApp::keyDown( KeyEvent event )
{
#if defined( CINDER_COCOA )
	switch ( event.getCode() ) {
		case KeyEvent::KEY_e:
			// export a new lut file to be edited in photoshop/gimp
			writeLookupTable( writeFile( getAssetPath( "" ) / "tempColorGrading.png" ), vec3(32), loadImage( loadAsset( "iceland.jpg" ) ) );
			
			// watch our lookup table for change
			wd::watch( "tempColorGrading.png", [this]( const fs::path &path ){
				readLookupTable( loadAsset( "tempColorGrading.png" ), vec3(32) );
			} );
			break;
	}
#endif
}
void ColorGradingApp::mouseDrag( MouseEvent event )
{
	mDiagonalTarget = event.getPos().x  / (float) getWindowWidth() + ( getWindowHeight() - event.getPos().y ) / (float) getWindowHeight();
}
void ColorGradingApp::mouseUp( MouseEvent event )
{
	mDiagonalTarget = 1.0f;
}

void ColorGradingApp::readLookupTable( const ci::DataSourceRef &lutImage, const ci::ivec3 &lutSize )
{
	mColorGradingLut		= gl::Texture3d::create( lutSize.z, lutSize.z, lutSize.z );
	auto surface	= Surface( loadImage( lutImage ) );
	for( int i = 0; i < lutSize.z; i++ ){
		mColorGradingLut->update( surface.clone( Area( ivec2( i * lutSize.z, 0 ), ivec2( i * lutSize.z + lutSize.x, lutSize.y ) ) ), i );
	}
}
ci::Surface8u ColorGradingApp::createLut( const ci::ivec3 &size )
{
	Surface lut = Surface( size.x * size.y, size.z, false, SurfaceChannelOrder::RGB );
	for( int i = 0; i < size.z; i++ ){
		for( int x = 0; x < size.x; x++ ){
			for( int y = 0; y < size.y; y++ ){
				lut.setPixel( ivec2( x + i * size.x, y ), ColorA( static_cast<float>( x ) / static_cast<float>( size.x ), static_cast<float>( y ) / static_cast<float>( size.y ), static_cast<float>( i ) / static_cast<float>( size.z ), 1.0f ) );
			}
		}
	}
	return lut;
}
void ColorGradingApp::writeLookupTable( const ci::DataTargetRef &lutImage, const ci::ivec3 &lutSize, const ci::ImageSourceRef &sourceImage, bool tryToOpenInPhotoshop )
{
	Surface lutSurface = createLut( lutSize );
	
	if( sourceImage ){
		Surface screenSurface = Surface( sourceImage );
		screenSurface.copyFrom( lutSurface, lutSurface.getBounds() );
		
		writeImage( lutImage, screenSurface, ImageTarget::Options().quality(1.0f) );
	}
	else writeImage( lutImage, lutSurface, ImageTarget::Options().quality(1.0f) );

#if defined( CINDER_COCOA )
	// try to find photoshop and open it
	if( tryToOpenInPhotoshop ){
		
		string photoshop;
		fs::directory_iterator end;
		fs::directory_iterator it( "/Applications" );
		while ( it != end ) {
			if( it->path().string().find( "Photoshop" ) != string::npos ){
				photoshop = it->path().stem().string();
			}
			it++;
		}
		
		if( !photoshop.empty() ){
			string command = "open -a \"" + photoshop + "\" " + lutImage->getFilePath().string();
			system( command.c_str() );
		}
	}
#endif
}

CINDER_APP( ColorGradingApp, RendererGl, []( App::Settings* settings ){
	settings->setWindowSize( ivec2( 2000, 1167 ) / 2 );
	settings->setMultiTouchEnabled(false);
})