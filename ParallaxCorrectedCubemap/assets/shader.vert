#version 410 core

uniform mat4 	ciModelView;
uniform mat4 	ciProjectionMatrix;
uniform mat3 	ciNormalMatrix;
uniform vec3	uCameraPosition;

in vec4			ciPosition;
in vec3			ciNormal;
in vec2 		ciTexCoord0;

out vec3		vPosition;
out vec3		vNormal;
out vec3		vReflection;
out vec2		vUv;

void main() {
	vPosition	= ( ciPosition ).xyz;
	vNormal		= ciNormal;
	vec3 dir 	= normalize( ciPosition.xyz - uCameraPosition );
	vReflection	= reflect( dir, normalize( ciNormal ) ); 
	vUv 		= ciTexCoord0;
	gl_Position = ciProjectionMatrix * ciModelView * ciPosition;
} 