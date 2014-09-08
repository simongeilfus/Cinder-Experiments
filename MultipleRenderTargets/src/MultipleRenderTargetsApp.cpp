#include "cinder/app/AppNative.h"
#include "cinder/app/RendererGl.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/VboMesh.h"
#include "cinder/gl/Shader.h"

#include "cinder/GeomIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MultipleRenderTargetsApp : public AppNative {
public:
    void setup() override;
    void draw() override;
    
    gl::FboRef      mFbo;
    gl::VboMeshRef  mMesh;
    gl::GlslProgRef mShader;
};

void MultipleRenderTargetsApp::setup()
{
    vec2 size = toPixels( getWindowSize() );
    
    // fbo format with 4 different type of textures and sizes
    gl::Fbo::Format fboFormat;
    fboFormat.attachment( GL_COLOR_ATTACHMENT0, gl::Texture2d::create( size.x, size.y, gl::Texture2d::Format().internalFormat( GL_RGBA ) ) );
    fboFormat.attachment( GL_COLOR_ATTACHMENT1, gl::Texture2d::create( size.x, size.y, gl::Texture2d::Format().internalFormat( GL_RGBA16F ) ) );
    fboFormat.attachment( GL_COLOR_ATTACHMENT2, gl::Texture2d::create( size.x, size.y, gl::Texture2d::Format().internalFormat( GL_R8 ) ) );
    fboFormat.attachment( GL_COLOR_ATTACHMENT3, gl::Texture2d::create( size.x * 0.75, size.y * 0.75 ) );
    fboFormat.samples( 16 );
    
    // create our fbo
    mFbo    = gl::Fbo::create( size.x, size.y, fboFormat );
    
    // create some mesh
    mMesh   = gl::VboMesh::create( geom::Cube().enable( geom::COLOR ) );
    
    // basic vertex shader
    const char* vert = R"(
#version 150
    
    uniform mat4    ciModelViewProjection;
    uniform mat4    ciModelView;
    uniform mat3    ciNormalMatrix;
    in vec4         ciPosition;
    in vec3         ciNormal;
    in vec4         ciColor;
    
    out vec4        vColor;
    out vec3        vNormal;
    out float       vDepth;
    
    void main( void ) {
        vColor      = ciColor;
        vNormal     = normalize( ciNormalMatrix * ciNormal );
        vDepth		=  -( ciModelView * ciPosition ).z / 100.0;
        gl_Position = ciModelViewProjection * ciPosition;
    }
    )";
    
    // basic fragment shader with 3 different output
    const char* frag = R"(
#version 150
#extension GL_ARB_explicit_attrib_location : enable
    
    in vec4     vColor;
    in vec3     vNormal;
    in float    vDepth;
    
    layout(location = 0) out vec4 attachment0;
    layout(location = 1) out vec4 attachment1;
    layout(location = 2) out vec4 attachment2;
    //layout(location = 3) out vec4 attachment3;
    
    void main( void ) {
        attachment0 = vColor;
        attachment1 = vec4( vNormal, 1.0 );
        attachment2 = vec4( vec3( vDepth ), 1.0 );
        //attachment3 = vec4( 0.3, 1.4, 1.4, 1.0 );
    }
    )";
    
    mShader = gl::GlslProg::create( vert, frag );
    
    gl::enableDepthWrite();
    gl::enableDepthRead();
}


void MultipleRenderTargetsApp::draw()
{
    // our 3d test scene
    auto renderScene = [this](){
        
        Rand r( 123456 );
        for( int i = 0; i < 125; i++ ){
            gl::pushModelView();
            gl::translate( r.nextVec3f() * r.nextFloat( 0.0f, 15.0f ) );
            gl::rotate( r.nextFloat() * 10.0f, r.nextVec3f() );
            gl::scale( r.nextVec3f() * r.nextFloat( 0.5f, 3.0f ) );
            gl::draw( mMesh );
            gl::popModelView();
        }
    };
    
    { // Render to the 3 first fbo attachments
        gl::ScopedFramebuffer fbo( mFbo );
        gl::ScopedGlslProg shader( mShader );
        
        // set the draw buffers to the 3 first attachments. if we don't
        // do that the render area will be limited to the smallest texture
        // attachment of the fbo, which is 0.75 * size (except if MSAA is enabled?)
        if( 1 ){
            GLenum drawBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers( 3, &drawBuffers[0] );
        }
        
        gl::clear( Color::black() );
        gl::viewport( mFbo->getSize() );
        gl::setMatrices( CameraPersp() );
        
        renderScene();
        
        // this will use the 3rd attachment of our fbo, but the 1st output
        // of our shader. because this attachment is smaller it will be scaled
        // up and only the lower left part of our scene will got rendered
        glDrawBuffer( GL_COLOR_ATTACHMENT3 );
        
        gl::clear( Color::black() );
        gl::enableWireframe();
        renderScene();
        gl::disableWireframe();
    }
    
    { // Draw one of the fbo attachment to the screen
        gl::clear( Color::black() );
        gl::setMatricesWindow( toPixels( getWindowSize() ) );
        gl::viewport( toPixels( getWindowSize() ) );
        
        // switch between the different color attachments
        int att = ci::math<int>::clamp( ( getMousePos().x - getWindowPos().x ) / (float) getWindowWidth() * 4, 0, 3 );
        gl::ScopedTextureBind texBind( mFbo->getTexture( GL_COLOR_ATTACHMENT0 + att ) );
        gl::ScopedGlslProg texShader( gl::getStockShader( gl::ShaderDef().color().texture() ) );
        gl::drawSolidRect( getWindowBounds() );
        
        getWindow()->setTitle( "Fbo Attachment " + toString( att ) + " | Framerate: " + toString( (int) getAverageFps() ) );
    }
    
}

CINDER_APP_NATIVE( MultipleRenderTargetsApp, RendererGl )
