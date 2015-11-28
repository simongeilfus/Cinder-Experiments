#version 410 core

uniform mat4 	ciModelView;
uniform mat4 	ciProjectionMatrix;
uniform mat3 	ciNormalMatrix;

in vec4			ciPosition;
in vec2 		ciTexCoord0;
in vec3			ciNormal;

out vec4		vPosition;
out vec3 		vNormal;
out vec3		vVsPosition;
out vec2		vUv;

void main() {
	vUv 			= ciTexCoord0;
	vec4 position 	= ciModelView * ciPosition;
	vPosition 		= ciPosition;
	vNormal			= normalize( ciNormalMatrix * ciNormal );
	vVsPosition 	= position.xyz;
	gl_Position		= ciProjectionMatrix * position;
} 