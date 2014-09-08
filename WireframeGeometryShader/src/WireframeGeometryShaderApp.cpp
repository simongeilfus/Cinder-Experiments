#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/ConstantStrings.h"
#include "cinder/GeomIo.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class WireframeGeometryShaderApp : public AppNative {
public:
    void setup();
    void draw();
    
    gl::VboMeshRef  mMesh;
    gl::GlslProgRef mShader;
};

void WireframeGeometryShaderApp::setup()
{
    // create our test mesh
    mMesh   = gl::VboMesh::create( geom::Icosphere().enable( geom::COLOR ) );
    
    // wrap the shader initialization in a lambda
    // so we can re-use it later to re-load on keydown.
    auto compileShaders = [this](){
        try {
            gl::GlslProg::Format format;
            format.vertex( loadAsset( "shader.vert" ) )
            .fragment( loadAsset( "shader.frag" ) )
            .geometry( loadAsset( "shader.geom" ) );
            
            mShader = gl::GlslProg::create( format );
        }
        catch( gl::GlslProgCompileExc exc ){
            cout << exc.what() << endl;
        }
    };
    
    // compile our shader
    compileShaders();
    
    // connect the keydown signal to our compileShader lambda.
    // easy way to update the shader on the fly.
    getWindow()->getSignalKeyDown().connect( [compileShaders](KeyEvent event) { compileShaders(); });
    
    gl::enableDepthWrite();
    gl::enableDepthRead();
}

void WireframeGeometryShaderApp::draw()
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
        gl::rotate( 5.0f * getElapsedSeconds(), vec3( 0.123, 0.456, 0.789 ) );
        gl::draw( mMesh );
    }
    
    getWindow()->setTitle( "Framerate: " + toString( (int) getAverageFps() ) );
    
}

CINDER_APP_NATIVE( WireframeGeometryShaderApp, RendererGl )
