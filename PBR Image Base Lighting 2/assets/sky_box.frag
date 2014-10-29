#version 150

uniform samplerCube uCubeMapTex;
uniform float		uExposure;
uniform float		uGamma;

in vec3	NormalWorldSpace;

out vec4 	oColor;


// Filmic tonemapping from
// http://filmicgames.com/archives/75

const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;

vec3 Uncharted2Tonemap( vec3 x )
{
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}


void main( void )
{
	vec3 color = textureLod( uCubeMapTex, NormalWorldSpace, 0 ).rgb;
	
	// apply the tone-mapping
	color					= Uncharted2Tonemap( color * uExposure );
	
	// white balance
	const float whiteInputLevel = 20.0f;
	vec3 whiteScale			= 1.0f / Uncharted2Tonemap( vec3( whiteInputLevel ) );
	color					= color * whiteScale;
	
	// gamma correction
	color					= pow( color, vec3( 1.0f / uGamma ) );
	
	oColor = vec4( color, 1.0 );
}