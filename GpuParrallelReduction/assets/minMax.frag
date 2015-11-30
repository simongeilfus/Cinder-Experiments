#version 410

uniform sampler2D uTex0;
uniform sampler2D uTex1;
uniform vec2 uInvSize;
uniform float uLod;

in vec2 vTexCoord0;

layout(location = 0) out vec4 oMax;

void main()
{
	float size 		= 1.0;
	vec4 sample0 	= texture( uTex0, vTexCoord0 + vec2(  size,  size ) * uInvSize );
	vec4 sample1 	= texture( uTex0, vTexCoord0 + vec2(  size, -size ) * uInvSize );
	vec4 sample2 	= texture( uTex0, vTexCoord0 + vec2( -size,  size ) * uInvSize );
	vec4 sample3 	= texture( uTex0, vTexCoord0 + vec2( -size, -size ) * uInvSize );

	oMax 			= max( sample0, max( sample1, max( sample2, sample3 ) ) );
}