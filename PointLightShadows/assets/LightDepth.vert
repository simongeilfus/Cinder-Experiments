#version 150

uniform mat4 	ciModelMatrix;
uniform mat4 	ciProjectionMatrix;
uniform mat4 	ciViewMatrix;

uniform vec3    uLightPosition;

in vec4         ciPosition;

out vec3 		vLightVector;

void main(){
	vec4 worldSpacePosition = ciModelMatrix * ciPosition;
	vLightVector 			= worldSpacePosition.xyz - uLightPosition;
	gl_Position 			= ciProjectionMatrix * ciViewMatrix * worldSpacePosition;
}