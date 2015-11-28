#version 410 core

uniform mat4 	ciModelView;
uniform mat4 	ciProjectionMatrix;

in vec4			ciPosition;
out vec3		vPosition;

void main()
{
	vec4 vsPos 	= ciModelView * ciPosition;
	vPosition	= vsPos.xyz;
	gl_Position = ciProjectionMatrix * vsPos;
} 