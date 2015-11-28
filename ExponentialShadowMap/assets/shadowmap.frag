#version 410 core

uniform float uLinearDepthScale;
uniform float uExpC;

in vec3		vPosition;
out float	oDepth;

void main() 
{
	// get the linear distance to the light
	// and store its exponential
	float linearDepth = -vPosition.z * uLinearDepthScale;
	oDepth = exp( uExpC * linearDepth );
}