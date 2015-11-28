#version 410 core

uniform int uSection;
uniform sampler2DArray uSampler;

in vec2		vUv;
out vec4	oColor;

void main() {
	oColor	= texture( uSampler, vec3( vUv, float( uSection ) ) );
}