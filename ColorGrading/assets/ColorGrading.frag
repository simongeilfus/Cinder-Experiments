#version 410 core

uniform sampler2D 	uSource;
uniform sampler3D 	uColorGradingLUT;
uniform float		uDiagonal;

in vec2           	vUv;
out vec4          	oColor;

void main(){
    vec3 color  = texture( uSource, vUv ).rgb;
	if( vUv.x + vUv.y > uDiagonal )
		color	= texture( uColorGradingLUT, color ).rgb;
    oColor      = vec4( color, 1.0f );
}