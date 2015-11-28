#version 410 core

uniform mat4 	ciModelMatrix;
uniform mat4 	ciModelView;
uniform mat4 	ciProjectionMatrix;
uniform mat3 	ciNormalMatrix;
uniform mat4 	uLightMatrix;

in vec4			ciPosition;
in vec3			ciNormal;

out vec3		vPosition;
out vec3 		vNormal;
out vec4 		vShadowCoord;

void main() {
	vec4 vsPosition = ciModelView * ciPosition;
	vPosition 		= vsPosition.xyz;
	vNormal			= normalize( ciNormalMatrix * ciNormal );
	vShadowCoord	= ( uLightMatrix * ciModelMatrix ) * ciPosition;
	gl_Position 	= ciProjectionMatrix * vsPosition;
} 