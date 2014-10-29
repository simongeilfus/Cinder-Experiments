#version 150

uniform mat4	ciModelViewProjection;

in vec4			ciPosition;

out highp vec3	NormalWorldSpace;

void main( void )
{
	NormalWorldSpace = normalize( vec3( ciPosition ) );
	gl_Position = ciModelViewProjection * ciPosition;
}


/*
uniform mat4    ciViewMatrix;
uniform mat4    ciProjectionMatrix;
uniform mat4    ciModelMatrix;
uniform mat3	ciNormalMatrix;
uniform mat4	ciViewMatrixInverse;


in vec4         ciPosition;
in vec3         ciNormal;

out vec3		vNormal;
out vec3		vPosition;
out vec3		vWsNormal;
out vec3		vEyePosition;
out vec3		vWsPosition;


void main(){
    vec4 worldSpacePosition	= ciModelMatrix * ciPosition;
    vec4 viewSpacePosition	= ciViewMatrix * worldSpacePosition;
	
    vNormal					= ciNormalMatrix * ciNormal;
    vPosition				= viewSpacePosition.xyz;
	vWsPosition				= worldSpacePosition.xyz;
	
	vec4 eyeDirViewSpace	= viewSpacePosition - vec4( 0, 0, 0, 1 );
	vEyePosition			= -vec3( ciViewMatrixInverse * eyeDirViewSpace );
	vWsNormal				= vec3( ciViewMatrixInverse * vec4( vNormal, 0.0 ) );
	
    gl_Position				= ciProjectionMatrix * viewSpacePosition;
}*/