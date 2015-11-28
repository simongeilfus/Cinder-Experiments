#version 150

uniform mat4    ciModelViewProjection;

in vec4         ciPosition;
in vec3         ciColor;

out vec3        vColor;

void main( void ) {
    // output the color
    vColor      = ciColor;
    // transform and output the position
    gl_Position = ciModelViewProjection * ciPosition;
}