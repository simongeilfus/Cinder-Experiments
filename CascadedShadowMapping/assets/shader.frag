#version 410 core

uniform float 			uCascadesNear[4];
uniform float 			uCascadesFar[4];
uniform vec2 			uCascadesPlanes[4];
uniform mat4 			uCascadesMatrices[4];

uniform sampler2D 		uAmbientOcclusion;
uniform sampler2DArray 	uShadowMap;

uniform vec3 			uLightDirection;
uniform mat4 			uOffsetMat;
uniform float 			uExpC;
uniform float 			uShowCascades;

in vec4		vPosition;
in vec3		vNormal;
in vec3		vVsPosition;
in vec2		vUv;

out vec4	oColor;

vec4 getCascadeWeights( float depth, vec4 splitNear, vec4 splitFar )
{
	vec4 near = step(splitNear, vec4(depth));
	vec4 far = step(depth, splitFar);
 
	return near * far;
}

mat4 getCascadeViewProjection( vec4 weights, mat4 viewProj[4] )
{
	return viewProj[0] * weights.x + viewProj[1] * weights.y + viewProj[2] * weights.z + viewProj[3] * weights.w;
}
 
float getCascadeZ( vec4 weights, float z[4] )
{
	return z[0] * weights.x + z[1] * weights.y + z[2] * weights.z + z[3] * weights.w;
}

float getCascadeLayer( vec4 weights ) 
{
	return 0.0 * weights.x + 1.0 * weights.y + 2.0 * weights.z + 3.0 * weights.w;	
}
float getCascadeNear( vec4 weights ) 
{
	return uCascadesNear[0] * weights.x + uCascadesNear[1] * weights.y + uCascadesNear[2] * weights.z + uCascadesNear[3] * weights.w;
}
float getCascadeFar( vec4 weights ) 
{
	return uCascadesFar[0] * weights.x + uCascadesFar[1] * weights.y + uCascadesFar[2] * weights.z + uCascadesFar[3] * weights.w;
}
vec3 getCascadeColor( vec4 weights ) 
{
	return vec3(1,0,0) * weights.x + vec3(0,1,0) * weights.y + vec3(0,0,1) * weights.z + vec3(1,0,1) * weights.w;
}

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
	vec3 V			= normalize( -vVsPosition );
	vec3 H			= normalize( V + L );

	// get all the usefull dot products and clamp them between 0 and 1 just to be safe
	float NoL		= saturate( dot( N, L ) );
	float NoV		= saturate( dot( N, V ) );
	float VoH		= saturate( dot( V, H ) );
	float NoH		= saturate( dot( N, H ) );

	// Find frustum section
	vec4 cascadeWeights = getCascadeWeights( -vVsPosition.z, 
		vec4( uCascadesPlanes[0].x, uCascadesPlanes[1].x, uCascadesPlanes[2].x, uCascadesPlanes[3].x ), 
		vec4( uCascadesPlanes[0].y, uCascadesPlanes[1].y, uCascadesPlanes[2].y, uCascadesPlanes[3].y ) );

	// get shadow coords
	mat4 viewProj = getCascadeViewProjection( cascadeWeights, uCascadesMatrices );
	vec4 shadowCoord = uOffsetMat * viewProj * vPosition;

	// calculate shadow term
	float near = getCascadeNear( cascadeWeights );
	float far = getCascadeFar( cascadeWeights );
	float depth = shadowCoord.z - 0.0052;
	float occluderDepth = texture( uShadowMap, vec3( shadowCoord.xy, getCascadeLayer( cascadeWeights ) ) ).r;
	float occluder = exp( uExpC * occluderDepth );
	float receiver = exp( -uExpC * depth );
	float shadows = clamp( occluder * receiver, 0.0, 1.0 );

	// deduce the diffuse and specular color from the baseColor and how metallic the material is
	vec3 uBaseColor = vec3( 1.0 );
	float uMetallic = 0.0;
	float uRoughness = 0.72;
	float uSpecular = 1.0;
	vec3 diffuseColor		= uBaseColor - uBaseColor * uMetallic;
	vec3 specularColor		= mix( vec3( 0.08 * uSpecular ), uBaseColor, uMetallic );
	
	// compute the brdf terms
	float distribution		= getNormalDistribution( uRoughness, NoH );
	vec3 fresnel			= getFresnel( specularColor, VoH );
	float geom				= getGeometricShadowing( uRoughness, NoV, NoL, VoH, L, V );

	// get the specular and diffuse and combine them
	vec3 diffuse			= getDiffuse( diffuseColor, uRoughness, NoV, NoL, VoH );
	vec3 specular			= NoL * ( distribution * fresnel * geom );
	vec3 color				= shadows * 3.0 * ( diffuse + specular );

	// gamma correction
	color					= pow( color, vec3( 1.0f / 2.2f ) );

	oColor = vec4( shadowCoord.xy, 0.0, 1.0 );
	//oColor = texture( mAmbientOcclusion, shadowCoord.xy );
	oColor = vec4( vec3( shadows * NoL ), 1.0 );
	oColor = vec4( color, 1.0 );
	//oColor = vec4( vec3( shadowCoord.z > occluderDepth + 0.0003 ? 0.0 : 1.0 ), 1.0 );
	//oColor = vec4( vec3( depth ), 1.0 );
	oColor.rgb *= texture( uAmbientOcclusion, vec2( vUv.x, 1.0 + vUv.y ) ).rgb;
	oColor.rgb = mix( oColor.rgb, oColor.rgb * getCascadeColor( cascadeWeights ), uShowCascades );

}