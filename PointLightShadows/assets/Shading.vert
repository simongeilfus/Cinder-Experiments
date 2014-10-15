#version 150

uniform mat4    ciViewMatrix;
uniform mat4    ciProjectionMatrix;
uniform mat4    ciModelMatrix;
//uniform mat3	ciNormalMatrix;

in vec4         ciPosition;
//in vec3         ciNormal;

//out vec3		vNormal;
out vec3		vPosition;

void main(){
    vec4 worldSpacePosition	= ciModelMatrix * ciPosition;
    vPosition				= worldSpacePosition.xyz;
    //vNormal				= ciNormalMatrix * ciNormal;
    gl_Position				= ciProjectionMatrix * ciViewMatrix * worldSpacePosition;
}