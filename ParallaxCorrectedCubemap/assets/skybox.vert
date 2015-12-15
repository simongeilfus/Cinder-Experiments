#version 410 core

uniform mat4 	ciModelView;
uniform mat4 	ciProjectionMatrix;
uniform mat4 	uOrientation;

in vec4			ciPosition;

out vec3		vPosition;

void main() {
	vPosition	= ( uOrientation * ciPosition ).xyz;
	gl_Position = ciProjectionMatrix * ciModelView * ciPosition;
} 