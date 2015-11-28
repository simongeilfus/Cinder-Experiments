//#version 410 core

uniform mat4 uView;
uniform mat4 ciModelView;
uniform mat4 ciProjectionMatrix;

in vec2 	ciTexCoord0;
in vec4		ciPosition;
out vec2	vUv;

void main() {
	vUv 			= ciTexCoord0;
	vec4 position 	= ciModelView * ciPosition;
	gl_Position		= ciProjectionMatrix * position;
} 