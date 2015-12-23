/* 
 Copyright (c) 2015, Simon Geilfus - All rights reserved. 
 
 This code is intended for use with the Cinder C++ library: http://libcinder.org
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/ObjLoader.h"
#include "glm/gtc/noise.hpp"
#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ParallaxCorrectedCubemapApp : public App {
  public:
	ParallaxCorrectedCubemapApp();
	void draw() override;
	
	void renderScene( bool withReflections = true );
	void renderCubemap();
	void userInterface();
	
	gl::BatchRef			mRoom, mSkyBox;
	gl::Texture2dRef		mLightMap, mRoughnessMap, mNormalMap;
	gl::FboCubeMapRef		mFboCubemap;
	gl::TextureCubeMapRef	mSkyBoxTexture;

	CameraPersp				mCamera;
	CameraUi				mCameraUi;
	vec3					mCubemapPosition;
	vec3					mCubemapSize;
	AxisAlignedBox			mCubemapBounds;
	
	bool					mShowUi, mDrawCubemapBounds, mDrawCubemap;
};

ParallaxCorrectedCubemapApp::ParallaxCorrectedCubemapApp()
{
	// setup the camera and camera ui
	mCamera = CameraPersp( getWindowWidth(), getWindowHeight(), 80.0f, 0.05f, 500.0f );
	mCamera.lookAt( vec3( 0.0f, 0.25f, 0.0f ), vec3( 0.0f, 0.0f, -1.0f ) );
	mCamera.setPivotDistance( 0.0f );
	mCameraUi = CameraUi( &mCamera, getWindow(), -1 );
	
	// load the test model, rescale it and create a batch with it
	TriMesh model( ObjLoader( loadAsset( "model.obj" ) ) );
	auto bounds = model.calcBoundingBox();
	auto shader = gl::GlslProg::create( loadAsset( "shader.vert" ), loadAsset( "shader.frag" ) );
	mRoom = gl::Batch::create( model >> geom::Scale( vec3( 1.0f / bounds.getSize().y ) ), shader );
	
	// load the material textures
	auto texFormat = gl::Texture2d::Format().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR ).mipmap();
	mLightMap = gl::Texture2d::create( loadImage( loadAsset( "lightMap.jpg" ) ), texFormat );
	mRoughnessMap = gl::Texture2d::create( loadImage( loadAsset( "roughness.jpg" ) ), texFormat );
	mNormalMap = gl::Texture2d::create( loadImage( loadAsset( "normal.png" ) ), texFormat );
	
	// setup a cubemap fbo to render the environment
	mFboCubemap = gl::FboCubeMap::create( 1024, 1024, gl::FboCubeMap::Format().textureCubeMapFormat( gl::TextureCubeMap::Format().mipmap().minFilter( GL_LINEAR_MIPMAP_LINEAR ).magFilter( GL_LINEAR ) ) );
	
	// load skybox texture and create a batch for it
	mSkyBoxTexture = gl::TextureCubeMap::create( loadImage( loadAsset( "output_skybox.jpg" ) ) );
	mSkyBox = gl::Batch::create( geom::Cube().size( vec3( 200.0f ) ), gl::GlslProg::create( loadAsset( "skybox.vert" ), loadAsset( "skybox.frag" ) ) );
	
	// set the environment map position and dimension
	vec3 cubemapPos( 0.0f, 0.2f, 0.0f );
	vec3 cubemapSize( 3.175f, 1.97f, 6.56f );
	mCubemapBounds.set( cubemapPos - cubemapSize * 0.5f, cubemapPos + cubemapSize * 0.5f );
	
	// setup ui
	ui::initialize();
	getWindow()->getSignalKeyDown().connect( [this]( KeyEvent event ) {
		if( event.getCode() == KeyEvent::KEY_SPACE ) mShowUi = !mShowUi;
	} );
	
	// initial options
	mShowUi				= false;
	mDrawCubemapBounds	= false;
	mDrawCubemap		= false;
	
	// renders the environment map
	renderCubemap();
}

void ParallaxCorrectedCubemapApp::draw()
{
	// clear the screen
	gl::clear( Color( 0, 0, 0 ) );
	
	// render the scene from the camera pov
	gl::setMatrices( mCamera );
	renderScene();
	
	// render the cubemap bounds
	if( mDrawCubemapBounds )
		gl::drawStrokedCube( mCubemapBounds );
	
	// render the cubemap texture
	if( mDrawCubemap ) {
		gl::ScopedDepth disableDepth( false );
		gl::setMatricesWindow( getWindowSize() );
		gl::drawHorizontalCross( mFboCubemap->getTextureCubeMap(), Rectf( vec2( 0.0f ), vec2( 256.0f ) ) );
	}
	
	// render the user interface
	if( mShowUi )
		userInterface();
	
	getWindow()->setTitle( "Parallax Corrected Environment Mapping | " + to_string( (int) getAverageFps() ) + " fps" );
}

void ParallaxCorrectedCubemapApp::renderScene( bool withReflections )
{
	// bind the different textures
	gl::ScopedTextureBind texBind0( mLightMap, 0 );
	gl::ScopedTextureBind texBind1( mFboCubemap->getTextureCubeMap(), 1 );
	gl::ScopedTextureBind texBind2( mRoughnessMap, 2 );
	gl::ScopedTextureBind texBind3( mNormalMap, 3 );
	
	auto shader = mRoom->getGlslProg();
	
	// disable reflections when rendering the cubemap
	if( withReflections )
		shader->uniform( "uReflections", 1.0f );
	else
		shader->uniform( "uReflections", 0.0f );
	
	// update the shader uniforms
	shader->uniform( "uLightMap", 0 );
	shader->uniform( "uCubeMap", 1 );
	shader->uniform( "uRoughnessMap", 2 );
	shader->uniform( "uNormalMap", 3 );
	shader->uniform( "uCameraPosition", vec3( mCamera.getEyePoint() ) );
	shader->uniform( "uCubeMapSize", mCubemapBounds.getSize() );
	shader->uniform( "uCubeMapPosition", mCubemapBounds.getCenter() );

	// enable backface culling and depth testing
	// GL_FRONT because the .obj model is wrong
	gl::ScopedFaceCulling cullBackFace( true, GL_FRONT );
	gl::ScopedDepth enableDepth( true );
	
	// render the room
	mRoom->draw();
	
	// remove the scale and translation from the view matrix
	// so the cubemap feels like infinitely far
	gl::ScopedFaceCulling disableCulling( false );
	gl::setViewMatrix( mat4( mat3( gl::getViewMatrix() ) ) );
	gl::ScopedTextureBind texBind4( mSkyBoxTexture, 0 );
	mSkyBox->getGlslProg()->uniform( "uOrientation", glm::rotate( -2.32f, vec3( 0, 1, 0 ) ) );
	mSkyBox->draw();
}

void ParallaxCorrectedCubemapApp::renderCubemap()
{
	// bind the cubemap fbo and set matrices and options
	gl::ScopedFramebuffer scopedFbo( mFboCubemap );
	gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboCubemap->getSize() );
	gl::ScopedMatrices scopedMatrices;
	gl::ScopedBlend disableBlend( false );
	gl::ScopedDepth enableDepth( true );
	
	// render each face of the cubemap
	for( GLenum dir = GL_TEXTURE_CUBE_MAP_POSITIVE_X; dir < GL_TEXTURE_CUBE_MAP_POSITIVE_X + 6; ++dir ) {
		mFboCubemap->bindFramebufferFace( dir );
		
		gl::clear();
		gl::setProjectionMatrix( ci::CameraPersp( mFboCubemap->getWidth(), mFboCubemap->getHeight(), 90.0f, mCamera.getNearClip(), mCamera.getFarClip() ).getProjectionMatrix() );
		gl::setViewMatrix( mFboCubemap->calcViewMatrix( dir, mCubemapBounds.getCenter() ) );
		renderScene( false );
	}
	
	// force the rendering of the mipmaps as we are using them to have cheap blurry reflections
	gl::ScopedTextureBind textureBind( mFboCubemap->getTextureCubeMap() );
	glGenerateMipmap( mFboCubemap->getTextureCubeMap()->getTarget() );
}
void ParallaxCorrectedCubemapApp::userInterface()
{
	ui::ScopedWindow window( "Parallax Corrected Environment Mapping" );
	
	ui::Checkbox( "Draw Cubemap Bounds", &mDrawCubemapBounds );
	ui::Checkbox( "Draw Cubemap", &mDrawCubemap );
	vec3 cubemapPos = mCubemapBounds.getCenter();
	vec3 cubemapSize = mCubemapBounds.getSize();
	if( ui::DragFloat3( "cubemapPos", &cubemapPos[0], 0.01f ) ||
	   ui::DragFloat3( "cubemapSize", &cubemapSize[0], 0.01f ) ) {
		mCubemapBounds.set( cubemapPos - cubemapSize * 0.5f, cubemapPos + cubemapSize * 0.5f );
		renderCubemap();
	}
}

CINDER_APP( ParallaxCorrectedCubemapApp, RendererGl( RendererGl::Options().msaa( 8 ) ), []( App::Settings* settings ) {
 settings->setWindowSize( 1280, 800 );
} )
