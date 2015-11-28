#version 150

uniform samplerCube uCubeMapTex;
uniform float		uExposure;
uniform float		uGamma;

in vec3		vDirection;

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
	vec3 color 	= pow( texture( uCubeMapTex, vDirection ).rgb, vec3( 2.2f ) );
	
	// apply the tone-mapping
	color 		= Uncharted2Tonemap( color * uExposure );
	// white balance
	color		= color * ( 1.0f / Uncharted2Tonemap( vec3( 20.0f ) ) );
	
	// gamma correction
	color		= pow( color, vec3( 1.0f / uGamma ) );
	oColor 		= vec4( color, 1.0 );
}