#version 150

out float oColor;

in vec3 vLightVector;

void main(){
	float depth = length( vLightVector );
	oColor = depth;
}