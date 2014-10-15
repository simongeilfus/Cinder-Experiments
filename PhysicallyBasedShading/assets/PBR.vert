#version 150

uniform mat4    ciViewMatrix;
uniform mat4    ciProjectionMatrix;
uniform mat4    ciModelMatrix;
uniform mat3	ciNormalMatrix;

uniform vec3    uLightPosition;

in vec4         ciPosition;
in vec3         ciNormal;

out vec3		vNormal;
out vec3		vLightPosition;
out vec3		vPosition;


void main(){
    vec4 worldSpacePosition	= ciModelMatrix * ciPosition;
    vec4 viewSpacePosition	= ciViewMatrix * worldSpacePosition;
	
    vNormal					= ciNormalMatrix * ciNormal;
    vLightPosition			= ( ciViewMatrix * vec4( uLightPosition, 1.0 ) ).xyz;
    vPosition				= viewSpacePosition.xyz;
	
    gl_Position				= ciProjectionMatrix * viewSpacePosition;
}