#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/ConstantStrings.h"

#include "cinder/GeomIo.h"
#include "cinder/Log.h"
#include "cinder/ObjLoader.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;


/* 
 Tessellation Shader from Philip Rideout
 
 "Triangle Tessellation with OpenGL 4.0"
 http://prideout.net/blog/?p=48 */

class TessellationShaderApp : public AppNative {
public:
    void setup();
    void draw();
    
    gl::VboMeshRef  mMesh;
    gl::GlslProgRef mShader;
    
    float mInnerLevel;
    float mOuterLevel;
};

void TessellationShaderApp::setup()
{    
    getWindow()->setAlwaysOnTop();
    
    // create our test mesh
    mMesh   = gl::VboMesh::create( geom::Icosahedron() );
    
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
    
    mInnerLevel = 1.0f;
    mOuterLevel = 1.0f;
    
    // connect the keydown signal to our compileShader lambda.
    // easy way to update the shader on the fly.
    getWindow()->getSignalKeyDown().connect( [this, compileShaders](KeyEvent event) {
        switch ( event.getCode() ) {
            case KeyEvent::KEY_r : compileShaders(); break;
            case KeyEvent::KEY_LEFT : mInnerLevel--; break;
            case KeyEvent::KEY_RIGHT : mInnerLevel++; break;
            case KeyEvent::KEY_DOWN : mOuterLevel--; break;
            case KeyEvent::KEY_UP : mOuterLevel++; break;
            case KeyEvent::KEY_1 : mMesh = gl::VboMesh::create( geom::Cube() ); break;
            case KeyEvent::KEY_2 : mMesh = gl::VboMesh::create( geom::Icosahedron() ); break;
            case KeyEvent::KEY_3 : mMesh = gl::VboMesh::create( geom::Cylinder() ); break;
            case KeyEvent::KEY_4 : mMesh = gl::VboMesh::create( geom::Cone() ); break;
            case KeyEvent::KEY_5 : mMesh = gl::VboMesh::create( geom::Sphere() ); break;
            case KeyEvent::KEY_6 : mMesh = gl::VboMesh::create( geom::Icosphere() ); break;

        }
        mInnerLevel = math<float>::max( mInnerLevel, 1.0f );
        mOuterLevel = math<float>::max( mOuterLevel, 1.0f );
    });
    
    gl::enableDepthWrite();
    gl::enableDepthRead();
}


void TessellationShaderApp::draw()
{
    // clear out the window with black
    gl::clear( Color( 0, 0, 0 ) );
    
    CameraPersp cam;
    cam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
    gl::setMatrices( cam );
    gl::viewport( getWindowSize() );
    
    // wrap the following calls in an if in case
    // we add some errors in the shader
    if( mShader ){
        gl::ScopedGlslProg shader( mShader );
        mShader->uniform( "TessLevelInner", mInnerLevel );
        mShader->uniform( "TessLevelOuter", mOuterLevel );
        
        gl::rotate( 5.0f * getElapsedSeconds(), vec3( 0.123, 0.456, 0.789 ) );
        
        auto ctx = gl::context();
        auto curGlslProg = ctx->getGlslProg();
        if( ! curGlslProg ) {
            CI_LOG_E( "No GLSL program bound" );
            return;
        }
        
        ctx->pushVao();
        ctx->getDefaultVao()->replacementBindBegin();
        mMesh->buildVao( curGlslProg );
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

CINDER_APP_NATIVE( TessellationShaderApp, RendererGl )
