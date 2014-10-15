#version 150

uniform samplerCube uDepthMap;
uniform vec3        uLightPosition;

in vec3             vNormal;
in vec3             vPosition;
in vec3             vViewSpaceLightPosition;
in vec3             vViewSpacePosition;

out vec4			oColor;

const vec2 poissonDisc[] = vec2[]( vec2( -0.94201624, -0.39906216 ), vec2( 0.94558609,  -0.76890725 ), vec2( -0.094184101, -0.92938870 ), vec2( 0.34495938, 0.29387760 ), vec2( -0.91588581, 0.45771432 ), vec2( -0.81544232, -0.87912464 ), vec2( -0.38277543,   0.27676845 ), vec2(  0.97484398, 0.75648379 ), vec2( 0.44323325, -0.97511554 ), vec2(  0.53742981, -0.47373420 ), vec2( -0.26496911, -0.41893023 ), vec2( 0.79197514,   0.19090188 ), vec2( -0.24188840, 0.99706507 ), vec2( -0.81409955, 0.91437590 ), vec2(  0.19984126, 0.78641367 ), vec2( 0.14383161, -0.14100790 ) );


void main() {
	vec3 positionToLight    = vPosition - uLightPosition;
	float dist				= length( positionToLight );
	
	// takes more samples to get softer shadows
	float shadow            = 0.0f;
	vec3 side				= normalize( cross( positionToLight, vec3(0,0,1) ) );
	vec3 up					= normalize( cross( side, positionToLight ) );
	
	const int samples		= 16;
	const float invSamples	= 1.0f / float( samples );
	const float eps         = 0.25f;
	const float jitter		= 0.05f;
	for( int i = 0; i < samples; i ++ ){
		vec3 dir			= positionToLight + side * poissonDisc[i].x * jitter + up * poissonDisc[i].y * jitter;
		float minDist		= texture( uDepthMap, dir ).x + eps;
		shadow				+= float( dist < minDist ) * invSamples;
	}

	// basic diffuse term with NoL
    vec3 n                  = normalize( vNormal );
    vec3 l                  = normalize( vViewSpaceLightPosition - vViewSpacePosition );
    float diffuse           = max( dot( n, l ), 0.0 );
	
    oColor                  = vec4( shadow * diffuse );
}