#version 150

uniform mat4    ciViewMatrix;
uniform mat4    ciProjectionMatrix;
uniform mat4    ciModelMatrix;
uniform mat3	ciNormalMatrix;

uniform vec3    uLightPositions[2];

in vec4         ciPosition;
in vec4         ciColor;
in vec3         ciNormal;
in vec2			ciTexCoord0;

out vec3		vNormal;
out vec3		vLightPositions[2];
out vec3		vPosition;
out vec2		vTexCoord;
out vec4		vColor;


void main(){
    vec4 worldSpacePosition	= ciModelMatrix * ciPosition;
    vec4 viewSpacePosition	= ciViewMatrix * worldSpacePosition;
	
	vNormal					= ciNormalMatrix * ciNormal;
	vLightPositions[0]		= ( ciViewMatrix * vec4( uLightPositions[0], 1.0 ) ).xyz;
	vLightPositions[1]		= ( ciViewMatrix * vec4( uLightPositions[1], 1.0 ) ).xyz;
    vPosition				= viewSpacePosition.xyz;
	vTexCoord				= ciTexCoord0;
	vColor					= ciColor;
	
    gl_Position				= ciProjectionMatrix * viewSpacePosition;
}