// passthrough fragment shader
#version 410 core
in vec3		vPosition;
in vec3		gColor;
out vec4	oColor;

void main() {
	oColor	= vec4( gColor, 1.0 );
}