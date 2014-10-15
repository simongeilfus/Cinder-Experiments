#version 150

uniform vec3        uLightPosition;
uniform vec3		uBaseColor;
uniform float		uRoughness;
uniform float		uMetallic;

in vec3             vNormal;
in vec3             vPosition;
in vec3             vViewSpaceLightPosition;
in vec3             vViewSpacePosition;

out vec4            oColor;

#define saturate(x) clamp(x, 0.0, 1.0)
#define PI 3.14159265359

// OrenNayar diffuse
vec3 getDiffuse( vec3 diffuseColor, float roughness, float NoV, float NoL, float VoH )
{
	float VoL = 2 * VoH - 1;
	float m = roughness * roughness;
	float m2 = m * m;
	float c1 = 1 - 0.5 * m2 / (m2 + 0.33);
	float cosri = VoL - NoV * NoL;
	float c2 = 0.45 * m2 / (m2 + 0.09) * cosri * ( cosri >= 0 ? min( 1, NoL / NoV ) : NoL );
	return diffuseColor / PI * ( NoL * c1 + c2 );
}

// GGX Normal distribution
float getNormalDistribution( float roughness, float NoH )
{
	float m = roughness * roughness;
	float m2 = m * m;
	float d = ( NoH * m2 - NoH ) * NoH + 1;
	return m2 / ( d*d );
}

// Smith GGX geometric shadowing from "Physically-Based Shading at Disney"
float getGeometricShadowing( float roughness, float NoV, float NoL, float VoH, vec3 L, vec3 V )
{
	float a = pow( roughness, 2 );
	float a2 = a*a;
	
	float gSmithV = NoV + sqrt( NoV * (NoV - NoV * a2) + a2 );
	float gSmithL = NoL + sqrt( NoL * (NoL - NoL * a2) + a2 );
	return gSmithV * gSmithL;
}

// Fresnel term
vec3 getFresnel( vec3 specularColor, float VoH )
{
	vec3 specularColorSqrt = sqrt( clamp( vec3(0, 0, 0), vec3(0.99, 0.99, 0.99), specularColor ) );
	vec3 n = ( 1 + specularColorSqrt ) / ( 1 - specularColorSqrt );
	vec3 g = sqrt( n*n + VoH*VoH - 1 );
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
const float W = 11.2;

vec3 Uncharted2Tonemap( vec3 x )
{
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}


void main() {
    vec3 positionToLight    = vPosition - uLightPosition;
	
	// get the normal, light, position and half vector normalized
	vec3 N                  = normalize( vNormal );
	vec3 L                  = normalize( vViewSpaceLightPosition - vViewSpacePosition );
	vec3 V                  = normalize( -vViewSpacePosition );
	vec3 H					= normalize(V + L);
	
	// get all the usefull dot products and clamp them between 0 and 1 just to be safe
	float NoL				= saturate( dot( N, L ) );
	float NoV				= saturate( dot( N, V ) );
	float VoH				= saturate( dot( V, H ) );
	float NoH				= saturate( dot( N, H ) );
	
	// deduce the specular color from the baseColor and how metallic the material is
	vec3 specularColor		= mix( vec3( 0.08 ), uBaseColor, uMetallic );
	
	// compute the brdf terms
	float distribution		= getNormalDistribution( uRoughness, NoH );
	vec3 fresnel			= getFresnel( specularColor, VoH );
	float geom				= getGeometricShadowing( uRoughness, NoV, NoL, VoH, L, V );

	// get the specular and diffuse and combine them
	vec3 diffuse			= getDiffuse( uBaseColor, uRoughness, NoV, NoL, VoH );
	vec3 specular			= NoL * ( distribution * fresnel * geom );
	vec3 color				= diffuse + specular;
	
	// fake some ambiant directional light with NoV
	color					+= vec3( NoV ) * 0.01f;
	
	// apply the tone-mapping
	float exposure			= 3.0f;
	color					= Uncharted2Tonemap( color * exposure);
	
	// white balance
	vec3 whiteScale			= 1.0f / Uncharted2Tonemap(vec3(W));
	color					= color * whiteScale;
	
	// gamma correction
	float gamma				= 2.2f;
	color					= pow( color, vec3( 1.0f / gamma ) );
	
	// output the fragment color
    oColor                  = vec4( color, 1.0 );
}