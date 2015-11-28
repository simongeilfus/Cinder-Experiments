// passthrough vertex shader
// no vertex transformation

#version 410 core
in vec4			ciPosition;
in vec3			ciColor;

out vec3		vColor;
void main() {
	vColor		= ciColor;
	gl_Position	= ciPosition;
} 