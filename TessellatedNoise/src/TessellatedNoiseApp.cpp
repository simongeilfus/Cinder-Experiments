#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/VboMesh.h"

#include "cinder/GeomIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;


/* 
 Tessellation Shader from Philip Rideout
 
 "Triangle Tessellation with OpenGL 4.0"
 http://prideout.net/blog/?p=48 */

class TessellatedNoise : public AppNative {
public:
    void setup();
    void draw();
    
    gl::VboMeshRef  mMesh;
    gl::GlslProgRef mShader;
    
    float mInnerLevel;
    float mOuterLevel;
    
    MayaCamUI mMayaCam;
};

namespace cinder { namespace gl {
    
    void namedString( const std::string &name, const std::string &code )
    {
        glNamedStringARB( GL_SHADER_INCLUDE_ARB, name.length(), name.c_str(), code.length(), code.c_str() );
    }
    
    void namedString( const std::string &name, const ci::DataSourceRef &source )
    {
        string code = loadString( source );
        glNamedStringARB( GL_SHADER_INCLUDE_ARB, name.length(), name.c_str(), code.length(), code.c_str() );
    }
    
    void namedString( const fs::path& filename )
    {
        try {
            string code = loadString( loadAsset( filename ) );
            string name = "/" + filename.string();
            glNamedStringARB( GL_SHADER_INCLUDE_ARB, name.length(), name.c_str(), code.length(), code.c_str() );
        }
        catch( AssetLoadExc exc ){
            cout << "Loading " << exc.what() << "Failed." << endl;
        }
    }
    
}}

void TessellatedNoise::setup()
{    
    getWindow()->setAlwaysOnTop();
    
    gl::namedString( "lighting.glsl" );
    gl::namedString( "noise.glsl" );
    gl::namedString( "nested.glsl" );
    
    
    // create our test mesh
    mMesh = gl::VboMesh::create( geom::Icosphere().subdivisions( 4 ) );
    
    // wrap the shader initialization in a lambda
    // so we can re-use it later to re-load on keydown.
    auto compileShaders = [this](){
        try {
            gl::GlslProg::Format format;
            format.vertex( loadAsset( "shader.vert" ) )
            .fragment( loadAsset( "shader.frag" ) )
            .geometry( loadAsset( "shader.geom" ) )
            .tessellation( loadAsset( "shader.cont" ), loadAsset( "shader.eval" ) );
            ;
            mShader = gl::GlslProg::create( format );
        }
        catch( gl::GlslProgCompileExc exc ){
            cout << exc.what() << endl;
        }
    };
    
    // compile our shader
    compileShaders();
    
    mInnerLevel = 8.0f;
    mOuterLevel = 6.0f;
    
    // connect the keydown signal to our compileShader lambda.
    // easy way to update the shader on the fly.
    getWindow()->getSignalKeyDown().connect( [this, compileShaders](KeyEvent event) {
        switch ( event.getCode() ) {
            case KeyEvent::KEY_r : compileShaders(); break;
            case KeyEvent::KEY_LEFT : mInnerLevel--; break;
            case KeyEvent::KEY_RIGHT : mInnerLevel++; break;
            case KeyEvent::KEY_DOWN : mOuterLevel--; break;
            case KeyEvent::KEY_UP : mOuterLevel++; break;

        }
        mInnerLevel = math<float>::max( mInnerLevel, 1.0f );
        mOuterLevel = math<float>::max( mOuterLevel, 1.0f );
    });
    
    // Mayacam init and mouse inputs
    CameraPersp cam;
    cam.setPerspective( 60, getWindowAspectRatio(), 0.1, 100 );
    cam.setEyePoint( vec3( 0, 0, -5.0f ) );
    cam.setCenterOfInterestPoint( vec3( 0 ) );
    mMayaCam.setCurrentCam( cam );
    
    getWindow()->getSignalMouseDown().connect( [this](MouseEvent event){
        mMayaCam.mouseDown( event.getPos() );
    });
    getWindow()->getSignalMouseDrag().connect( [this](MouseEvent event){
        mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
    });
    
    gl::enableDepthWrite();
    gl::enableDepthRead();
}


void TessellatedNoise::draw()
{
    gl::clear( Color::black() );
    
    gl::setMatrices( mMayaCam.getCamera() );
    gl::viewport( getWindowSize() );
    
    vec4 lightPosition = mMayaCam.getCamera().getViewMatrix() * ( angleAxis( /*(float) getElapsedSeconds() */ 0.05f, vec3( 0.0f, 1.0f, 0.0f ) ) * vec4( 120.0f, 0.0f, 0.0f, 1.0 ) );
    
    if( mShader ){
        gl::ScopedGlslProg shader( mShader );
        mShader->uniform( "uTessLevelInner", mInnerLevel );
        mShader->uniform( "uTessLevelOuter", mOuterLevel );
        mShader->uniform( "uTime", (float) getElapsedSeconds() );
        mShader->uniform( "uLightPosition", vec3( lightPosition ) );
        
        auto ctx = gl::context();
        
        ctx->pushVao();
        ctx->getDefaultVao()->replacementBindBegin();
        mMesh->buildVao( mShader );
        ctx->getDefaultVao()->replacementBindEnd();
        ctx->setDefaultShaderVars();
        
        if( mMesh->getNumIndices() )
            glDrawElements( GL_PATCHES, mMesh->getNumIndices(), mMesh->getIndexDataType(), (GLvoid*)( 0 ) );
        else
            glDrawArrays( GL_PATCHES, 0, mMesh->getNumIndices() );
        
        ctx->popVao();
    }
    
    getWindow()->setTitle( "Framerate: " + toString( (int) getAverageFps() ) );
}

CINDER_APP_NATIVE( TessellatedNoise, RendererGl )
