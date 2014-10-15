#version 150

uniform mat4    ciViewMatrix;
uniform mat4    ciProjectionMatrix;
uniform mat4    ciModelMatrix;
uniform mat3	ciNormalMatrix;

uniform vec3    uLightPosition;

in vec4         ciPosition;
in vec3         ciNormal;

out vec3		vNormal;
out vec3		vPosition;
out vec3		vViewSpaceLightPosition;
out vec3		vViewSpacePosition;

void main(){
    vec4 worldSpacePosition	= ciModelMatrix * ciPosition;
    vec4 viewSpacePosition	= ciViewMatrix * worldSpacePosition;
	
	vViewSpaceLightPosition	= ( ciViewMatrix * vec4( uLightPosition, 1.0 ) ).xyz;
	vViewSpacePosition 		= viewSpacePosition.xyz;
	
    vPosition				= worldSpacePosition.xyz;
    vNormal					= ciNormalMatrix * ciNormal;
    gl_Position				= ciProjectionMatrix * viewSpacePosition;
}