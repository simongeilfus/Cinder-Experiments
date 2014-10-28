#version 150

uniform vec3        uLightPosition;
uniform vec3        uLightColor;
uniform float       uLightRadius;

uniform vec3		uBaseColor;
uniform float		uRoughness;
uniform float		uMetallic;
uniform float		uSpecular;

uniform float		uExposure;
uniform float		uGamma;

in vec3             vNormal;
in vec3             vLightPosition;
in vec3             vPosition;

out vec4            oColor;

#define saturate(x) clamp(x, 0.0, 1.0)
#define PI 3.14159265359

// OrenNayar diffuse
vec3 getDiffuse( vec3 diffuseColor, float roughness4, float NoV, float NoL, float VoH )
{
	float VoL = 2 * VoH - 1;
	float c1 = 1 - 0.5 * roughness4 / (roughness4 + 0.33);
	float cosri = VoL - NoV * NoL;
	float c2 = 0.45 * roughness4 / (roughness4 + 0.09) * cosri * ( cosri >= 0 ? min( 1, NoL / NoV ) : NoL );
	return diffuseColor / PI * ( NoL * c1 + c2 );
}

// GGX Normal distribution
float getNormalDistribution( float roughness4, float NoH )
{
	float d = ( NoH * roughness4 - NoH ) * NoH + 1;
	return roughness4 / ( d*d );
}

// Smith GGX geometric shadowing from "Physically-Based Shading at Disney"
float getGeometricShadowing( float roughness4, float NoV, float NoL, float VoH, vec3 L, vec3 V )
{	
	float gSmithV = NoV + sqrt( NoV * (NoV - NoV * roughness4) + roughness4 );
	float gSmithL = NoL + sqrt( NoL * (NoL - NoL * roughness4) + roughness4 );
	return 1.0 / ( gSmithV * gSmithL );
}

// Fresnel term
vec3 getFresnel( vec3 specularColor, float VoH )
{
	vec3 specularColorSqrt = sqrt( clamp( vec3(0, 0, 0), vec3(0.99, 0.99, 0.99), specularColor ) );
	vec3 n = ( 1 + specularColorSqrt ) / ( 1 - specularColorSqrt );
	vec3 g = sqrt( n * n + VoH * VoH - 1 );
	return 0.5 * pow( (g - VoH) / (g + VoH), vec3(2.0) ) * ( 1 + pow( ((g+VoH)*VoH - 1) / ((g-VoH)*VoH + 1), vec3(2.0) ) );
}

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

// From "I'm doing it wrong"
// http://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
float getAttenuation( vec3 lightPosition, vec3 vertexPosition, float lightRadius )
{
	float r				= lightRadius;
	vec3 L				= lightPosition - vertexPosition;
	float dist			= length(L);
	float d				= max( dist - r, 0 );
	L					/= dist;
	float denom			= d / r + 1.0f;
	float attenuation	= 1.0f / (denom*denom);
	float cutoff		= 0.0052f;
	attenuation			= (attenuation - cutoff) / (1 - cutoff);
	attenuation			= max(attenuation, 0);
	
	return attenuation;
}

// https://www.shadertoy.com/view/4ssXRX
//note: uniformly distributed, normalized rand, [0;1[
float nrand( vec2 n )
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}
float random( vec2 n, float seed )
{
	float t = fract( seed );
	float nrnd0 = nrand( n + 0.07*t );
	float nrnd1 = nrand( n + 0.11*t );
	float nrnd2 = nrand( n + 0.13*t );
	float nrnd3 = nrand( n + 0.17*t );
	return (nrnd0+nrnd1+nrnd2+nrnd3) / 4.0;
}

void main() {
	// get the normal, light, position and half vector normalized
	vec3 N                  = normalize( vNormal );
	vec3 L                  = normalize( vLightPosition - vPosition );
	vec3 V                  = normalize( -vPosition );
	vec3 H					= normalize(V + L);
	
	// get all the usefull dot products and clamp them between 0 and 1 just to be safe
	float NoL				= saturate( dot( N, L ) );
	float NoV				= saturate( dot( N, V ) );
	float VoH				= saturate( dot( V, H ) );
	float NoH				= saturate( dot( N, H ) );
	
	//
	vec3 diffuseColor		= uBaseColor - uBaseColor * uMetallic;
	
	// deduce the specular color from the baseColor and how metallic the material is
	vec3 specularColor		= mix( vec3( 0.08 * uSpecular ), uBaseColor, uMetallic );
	
	// compute the brdf terms
	float distribution		= getNormalDistribution( uRoughness, NoH );
	vec3 fresnel			= getFresnel( specularColor, VoH );
	float geom				= getGeometricShadowing( uRoughness, NoV, NoL, VoH, L, V );

	// get the specular and diffuse and combine them
	vec3 diffuse			= getDiffuse( diffuseColor, uRoughness, NoV, NoL, VoH );
	vec3 specular			= NoL * ( distribution * fresnel * geom );
	vec3 color				= uLightColor * ( diffuse + specular );
	
	// get the light attenuation from its radius
	float attenuation		= getAttenuation( vLightPosition, vPosition, uLightRadius );
	color					*= attenuation;
	
	// apply the tone-mapping
	color					= Uncharted2Tonemap( color * uExposure );
	
	// white balance
	const float whiteInputLevel = 2.0f;
	vec3 whiteScale			= 1.0f / Uncharted2Tonemap( vec3( whiteInputLevel ) );
	color					= color * whiteScale;
	
	// gamma correction
	color					= pow( color, vec3( 1.0f / uGamma ) );
	
	// output the fragment color
    oColor                  = vec4( color, 1.0 );
}