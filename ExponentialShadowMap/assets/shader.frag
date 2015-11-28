#version 410 core

uniform sampler2D 	uShadowMap;
uniform vec3 		uLightPosition;
uniform vec3 		uLightDirection;
uniform float 		uLinearDepthScale;
uniform float 		uExpC;
uniform float 		uErrorEPS;
uniform float 		uShowErrors;

uniform vec3		uBaseColor;
uniform float		uRoughness;
uniform float		uMetallic;
uniform float		uSpecular;

in vec3				vPosition;
in vec3				vNormal;
in vec4		 		vShadowCoord;

out vec4			oColor;

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


void main() {
	// get the normal, light, position and half vector normalized
	vec3 N 			= normalize( vNormal );
	vec3 L 			= normalize( -uLightDirection );
	vec3 V			= normalize( -vPosition );
	vec3 H			= normalize( V + L );

	// get all the usefull dot products and clamp them between 0 and 1 just to be safe
	float NoL		= saturate( dot( N, L ) );
	float NoV		= saturate( dot( N, V ) );
	float VoH		= saturate( dot( V, H ) );
	float NoH		= saturate( dot( N, H ) );

	// shadow mapping
	vec3 coord 		= vShadowCoord.xyz / vShadowCoord.w;
	float shadows 	= 1.0;
	vec3 wrong 		= vec3( 0.0 );
	if ( coord.z > 0.0 && coord.x > 0.0 && coord.y > 0 && coord.x <= 1 && coord.y <= 1 ) {
		float depth 	= vShadowCoord.z * uLinearDepthScale;
		float occluder 	= texture( uShadowMap, coord.xy ).r;
		float receiver 	= exp( -uExpC * depth );

		// this is the shadow test!
	    shadows 		= occluder * receiver;

	    // darken the shadows a tiny bit more
	    shadows = clamp( pow( shadows, 3.0 ), 0.0, 1.0 );

	    // TODO: should implement a fallback when this fail
	    // apparently PCF should be the way to go I guess
	    if( shadows > 1.0 + uErrorEPS && uShowErrors > 0.0 ) {
	    	wrong = vec3(0.5,0,0);
	    	const float s = 1.0;
	    	float lod = 1.0;
			occluder = textureLodOffset( uShadowMap, coord.xy, lod, ivec2(-s,-s) ).r;
			occluder += textureLodOffset( uShadowMap, coord.xy, lod, ivec2(-s, 0) ).r;
			occluder += textureLodOffset( uShadowMap, coord.xy, lod, ivec2(-s, s) ).r;
			occluder += textureLodOffset( uShadowMap, coord.xy, lod, ivec2( 0,-s) ).r;
			occluder += textureLodOffset( uShadowMap, coord.xy, lod, ivec2( 0, 0) ).r;
			occluder += textureLodOffset( uShadowMap, coord.xy, lod, ivec2( 0, s) ).r;
			occluder += textureLodOffset( uShadowMap, coord.xy, lod, ivec2( s,-s) ).r;
			occluder += textureLodOffset( uShadowMap, coord.xy, lod, ivec2( s, 0) ).r;
			occluder += textureLodOffset( uShadowMap, coord.xy, lod, ivec2( s, s) ).r;
			occluder /= 9.0;
			
			//shadows = occluder * receiver;
	    }
	}

	// deduce the diffuse and specular color from the baseColor and how metallic the material is
	vec3 diffuseColor		= uBaseColor - uBaseColor * uMetallic;
	vec3 specularColor		= mix( vec3( 0.08 * uSpecular ), uBaseColor, uMetallic );
	
	// compute the brdf terms
	float distribution		= getNormalDistribution( uRoughness, NoH );
	vec3 fresnel			= getFresnel( specularColor, VoH );
	float geom				= getGeometricShadowing( uRoughness, NoV, NoL, VoH, L, V );

	// get the specular and diffuse and combine them
	vec3 diffuse			= getDiffuse( diffuseColor, uRoughness, NoV, NoL, VoH );
	vec3 specular			= NoL * ( distribution * fresnel * geom );
	vec3 color				= shadows * ( diffuse + specular );

	// gamma correction
	color					= pow( color, vec3( 1.0f / 2.2f ) );

    oColor 					= vec4( uShowErrors * wrong + color, 1.0 );
}