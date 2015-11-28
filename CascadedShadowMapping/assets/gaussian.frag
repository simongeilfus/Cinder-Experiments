#version 410 core

#define KERNEL_7x7_GAUSSIAN 2
#define KERNEL_11x11_GAUSSIAN 3
#define KERNEL_15x15_GAUSSIAN 4

#ifndef KERNEL
	#define KERNEL KERNEL_7x7_GAUSSIAN
#endif

#if KERNEL == KERNEL_7x7_GAUSSIAN
	const float offsets[KERNEL] = float[KERNEL]( 0.538049, 2.06278 );
	const float weights[KERNEL] = float[KERNEL]( 0.44908, 0.0509202 );
#elif KERNEL == KERNEL_11x11_GAUSSIAN
	const float offsets[KERNEL] = float[KERNEL]( 0.621839, 2.2731, 4.14653 );
	const float weights[KERNEL] = float[KERNEL]( 0.330228, 0.157012, 0.0127605 );
#elif KERNEL == KERNEL_15x15_GAUSSIAN
	const float offsets[KERNEL] = float[KERNEL]( 0.644342, 2.37885, 4.29111, 6.21661 );
	const float weights[KERNEL] = float[KERNEL]( 0.249615, 0.192463, 0.0514763, 0.00644572 );
#endif

uniform sampler2DArray 	uSampler;
uniform vec2 			uInvSize;
uniform vec2 			uDirection;

in float 				gLayer;
out float				oDepth;

void main() 
{
    oDepth 		= 0.0;
	vec2 coord 	= gl_FragCoord.xy * uInvSize;
	for( int i = 0; i < KERNEL; i++ ) {
        vec2 texCoordOffset = offsets[i] * uInvSize * uDirection;
        float depth = texture( uSampler, vec3( coord + texCoordOffset, gLayer ) ).x 
        			+ texture( uSampler, vec3( coord - texCoordOffset, gLayer ) ).x;
        oDepth += weights[i] * depth;
    }
}