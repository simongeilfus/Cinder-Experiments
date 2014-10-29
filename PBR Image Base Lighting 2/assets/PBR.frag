#version 150

uniform vec3        uLightColors[2];
uniform float       uLightRadiuses[2];

uniform samplerCube uCubeMapTex;

uniform vec3		uBaseColor;
uniform float		uRoughness;
uniform float		uRoughness4;
uniform float		uMetallic;
uniform float		uSpecular;
uniform float		uDetails;

uniform float		uExposure;
uniform float		uGamma;

uniform bool		uMask;
uniform bool		uDisplayBackground;

uniform sampler2D	uBaseColorMap;
uniform sampler2D	uNormalMap;
uniform sampler2D	uRoughnessMap;
uniform sampler2D	uMetallicMap;
uniform sampler2D	uEmissiveMap;

in vec3             vNormal;
in vec3             vLightPositions[2];
in vec3             vPosition;
in vec2				vTexCoord;
in vec4				vColor;
in vec3				vEyePosition;
in vec3				vWsNormal;
in vec3				vWsPosition;

out vec4            oColor;

#define saturate(x) clamp(x, 0.0, 1.0)
#define PI 3.1415926535897932384626433832795


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

// https://www.unrealengine.com/blog/physically-based-shading-on-mobile
vec3 EnvBRDFApprox( vec3 SpecularColor, float Roughness, float NoV )
{
	const vec4 c0 = vec4( -1, -0.0275, -0.572, 0.022 );
	const vec4 c1 = vec4( 1, 0.0425, 1.04, -0.04 );
	vec4 r = Roughness * c0 + c1;
	float a004 = min( r.x * r.x, exp2( -9.28 * NoV ) ) * r.x + r.y;
	vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;
	return SpecularColor * AB.x + AB.y;
}


// http://the-witness.net/news/2012/02/seamless-cube-map-filtering/
vec3 fix_cube_lookup( vec3 v, float cube_size, float lod ) {
	float M = max(max(abs(v.x), abs(v.y)), abs(v.z));
	float scale = 1 - exp2(lod) / cube_size;
	if (abs(v.x) != M) v.x *= scale;
	if (abs(v.y) != M) v.y *= scale;
	if (abs(v.z) != M) v.z *= scale;
	return v;
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


// Normal Blending (Unity normal blending)
// Source adapted from http://blog.selfshadow.com/publications/blending-in-detail/
vec3 blendNormals( vec3 baseNormal, vec3 detailsNormal )
{
	vec3 n1 = baseNormal;
	vec3 n2 = detailsNormal;
	mat3 nBasis = mat3(
        vec3(n1.z, n1.y, -n1.x), // +90 degree rotation around y axis
        vec3(n1.x, n1.z, -n1.y), // -90 degree rotation around x axis
        vec3(n1.x, n1.y,  n1.z));
	return normalize(n2.x*nBasis[0] + n2.y*nBasis[1] + n2.z*nBasis[2]);
}


// Scattering Implementation
// Code From Miles Macklin's
// http://blog.mmacklin.com/2010/05/29/in-scattering-demo/
float getScattering( vec3 dir, vec3 lightPos, float d)
{
	// calculate quadratic coefficients a,b,c
	vec3 q = - lightPos;
	
	float b = dot(dir, q);
	float c = dot(q, q);
	
	// evaluate integral
	float s = 1.0f / sqrt(c - b * b );
	
	return s * (atan( (d + b) * s) - atan( b*s ));
}


void main() {
	vec2 uv					= vTexCoord;
	
	vec3 normalDetails		= texture( uNormalMap, uv ).xyz * 2.0 - 1.0;
	normalDetails.x			*= -1.0f;
	normalDetails.y			*= -1.0f;
	normalDetails			= mix( vec3( 0.0,0.0,1), normalDetails, uDetails );
	
	// get the normal, light, position and half vector normalized
	vec3 N                  = blendNormals( vNormal, normalDetails );
	vec3 V					= normalize( -vPosition );
	
	vec3 wsN                = blendNormals( vWsNormal, normalDetails );
	vec3 wsV                = normalize( vEyePosition );
	
	vec3 baseColor			= texture( uBaseColorMap, uv ).rgb * uBaseColor;
	float roughness			= saturate( texture( uRoughnessMap, uv ).x * uRoughness );
	float roughness4		= saturate( pow( texture( uRoughnessMap, uv ).x, 4 ) * uRoughness4 );
	float metallic			= saturate( texture( uMetallicMap, uv ).x * uMetallic );
	//float ao				= pow( texture( uAoMap, uv ).x, 2.0 );
	//
	vec3 diffuseColor		= baseColor - baseColor * metallic;
	// deduce the specular color from the baseColor and how metallic the material is
	vec3 specularColor		= mix( vec3( 0.08 * uSpecular ), baseColor, metallic );
	
	vec3 color				= vec3( 0.0 );
	
	if( uMask && uDisplayBackground ) {
		color				= textureLod( uCubeMapTex, vWsNormal, 0.5 ).rgb;
	}
	
	// don't apply the specularIBL and emissivity to the background
	else if( !uMask ){
		
		// sample the pre-filtered cubemap at the corresponding mipmap level
		int numMips				= 6;
		float mip				= numMips - 1 + log2(roughness);
		vec3 lookup				= -reflect( wsV, wsN );
		//lookup					= fix_cube_lookup( lookup, 512, mip );
		vec3 sampledColor		= textureLod( uCubeMapTex, lookup, mip ).rgb;
		
		// get the approximate reflectance
		float wsNoV				= saturate( dot( wsN, wsV ) );
		vec3 reflectance		= EnvBRDFApprox( specularColor, roughness4, wsNoV );
		
		// cheat by tweaking the reflectance
		//reflectance				= pow( reflectance, vec3( 1.5 ) );
		
		// combine the specular IBL and the BRDF
		vec3 specularIBL		= sampledColor * reflectance * uSpecular;
		
		// still have to figure out how to do env. irradiance
		vec3 diffuseIBL			= textureLod( uCubeMapTex, vWsNormal, 5 ).rgb * ( 1 - 1 * metallic ) * diffuseColor;
		
		// not sure how to combine this with the rest
		color					+= diffuseIBL + specularIBL;
		
		// add material emissivity
		color					+= texture( uEmissiveMap, uv ).rgb * 0.5 * vColor.rgb;
	}
	
	for( int i = 1; i < 2; i++ ){
		
		vec3 L              = normalize( vLightPositions[i] - vPosition );
		vec3 H				= normalize(V + L);
		
		// get all the usefull dot products and clamp them between 0 and 1 just to be safe
		float NoL			= saturate( dot( N, L ) );
		float NoV			= saturate( dot( N, V ) );
		float VoH			= saturate( dot( V, H ) );
		float NoH			= saturate( dot( N, H ) );
		
		// compute the brdf terms
		float distribution	= getNormalDistribution( roughness, NoH );
		vec3 fresnel		= getFresnel( specularColor, VoH );
		float geom			= getGeometricShadowing( roughness, NoV, NoL, VoH, L, V );
		
		// get the specular and diffuse and combine them
		vec3 diffuse		= getDiffuse( diffuseColor, roughness, NoV, NoL, VoH );
		vec3 specular		= NoL * ( distribution * fresnel * geom );
		vec3 directLighting	= uLightColors[i] * ( diffuse + specular );
		
		// get the light attenuation from its radius
		float attenuation	= getAttenuation( vLightPositions[i], vPosition, uLightRadiuses[i] );
		color				+= attenuation * directLighting;
		
		// add in-scattering coeff
		color				+= saturate( pow( getScattering( -V, vLightPositions[i], -vPosition.z ), 1.25 ) * uLightColors[i] * uLightRadiuses[i] * 0.1 );
	}

	
	// apply the tone-mapping
	color					= Uncharted2Tonemap( color * uExposure );
	
	// white balance
	const float whiteInputLevel = 20.0f;
	vec3 whiteScale			= 1.0f / Uncharted2Tonemap( vec3( whiteInputLevel ) );
	color					= color * whiteScale;
	
	// gamma correction
	color					= pow( color, vec3( 1.0f / uGamma ) );
	
	// output the fragment color
    oColor                  = vec4( color, 1.0 );
}