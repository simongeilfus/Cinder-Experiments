#version 410 core

uniform float 	uCascadesNear[4];
uniform float 	uCascadesFar[4];
uniform float 	uExpC;

in float		gLayer;
in vec3			vsPosition;
out float 		oDepth;

void main() {
	int layer = int(gLayer);
	float linearDepth = ( -vsPosition.z - uCascadesNear[layer] ) / ( uCascadesFar[layer] - uCascadesNear[layer] );
	oDepth = ( linearDepth );
}