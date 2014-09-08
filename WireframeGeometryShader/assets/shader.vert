#version 150

uniform mat4    ciModelViewProjection;

in vec4         ciPosition;
in vec3         ciColor;

out vec3        vColor;

void main( void ) {
    // output the color
    vColor      = ciColor;
    
    // scale up the vertices
    vec4 position = vec4( ciPosition.xyz * 20.0, 1.0 );
    
    // transform and output the position
    gl_Position = ciModelViewProjection * position;
}