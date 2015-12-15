#version 410 core

uniform samplerCube	uCubeMap;

in vec3		vPosition;
out vec4	oColor;

void main() {
    oColor = pow( texture( uCubeMap, vPosition ), vec4( 0.1 ) );
}