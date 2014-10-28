#version 150

uniform samplerCube uCubeMapTex;

uniform vec3		uBaseColor;
uniform float		uRoughness;
uniform float		uMetallic;
uniform float		uSpecular;

uniform float		uExposure;
uniform float		uGamma;

in vec3             vNormal;
in vec3             vPosition;
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
float getGeometricShadowing( float roughness4, float NoV, float NoL )
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


float hash( vec2 p )
{
	float h = dot(p,vec2(127.1,311.7));
	return -1.0 + 2.0*fract(sin(h)*43758.5453123);
}

vec2 random( vec2 p )
{
	return vec2( hash( p.xy ), hash( p.yx ) );
}

// Interesting page on Hammersley Points
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html#
vec2 Hammersley(const in int index, const in int numSamples, ivec2 rand ){
	int reversedIndex = index;
	reversedIndex = (reversedIndex << 16) | (reversedIndex >> 16);
	reversedIndex = ((reversedIndex & 0x00ff00ff) << 8) | ((reversedIndex & 0xff00ff00) >> 8);
	reversedIndex = ((reversedIndex & 0x0f0f0f0f) << 4) | ((reversedIndex & 0xf0f0f0f0) >> 4);
	reversedIndex = ((reversedIndex & 0x33333333) << 2) | ((reversedIndex & 0xcccccccc) >> 2);
	reversedIndex = ((reversedIndex & 0x55555555) << 1) | ((reversedIndex & 0xaaaaaaaa) >> 1);
	
	return vec2(fract(float(index) / numSamples + float( rand.x & 0xffff ) / (1<<16) ), float(reversedIndex ^ rand.y ) * 2.3283064365386963e-10);
}

// straight from Epic paper for Siggraph 2013 Shading course
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf

vec3 ImportanceSampleGGX( vec2 Xi, float Roughness4, vec3 N ) {
	float Phi = 2.0f * PI * Xi.x;
	float CosTheta = sqrt( (1.0f - Xi.y) / ( 1.0f + (Roughness4 - 1.0f) * Xi.y ) );
	float SinTheta = sqrt( 1.0f - CosTheta * CosTheta );
	
	vec3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;
	
	vec3 UpVector = abs( N.z ) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
	vec3 TangentX = normalize( cross( UpVector, N ) );
	vec3 TangentY = cross( N, TangentX );
	
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

// http://graphicrants.blogspot.com.au/2013/08/specular-brdf-reference.html
float GGX(float NdotV, float a)
{
	float k = a / 2;
	return NdotV / (NdotV * (1.0f - k) + k);
}

// http://graphicrants.blogspot.com.au/2013/08/specular-brdf-reference.html
float G_Smith(float a, float nDotV, float nDotL)
{
	return GGX(nDotL, a) * GGX(nDotV, a);
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


vec3 SpecularIBL( vec3 SpecularColor, float Roughness, vec3 N, vec3 V, ivec2 rand ) {
	vec3 SpecularLighting = vec3(0);
	float NoV = saturate( dot( N, V ) );
	NoV = max( 0.001, NoV );
	const int NumSamples = 16;
	for( int i = 0; i < NumSamples; i++ ) {
		vec2 Xi = Hammersley( i, NumSamples, rand );
		vec3 H = ImportanceSampleGGX( Xi, Roughness, N );
		
		vec3 L = -reflect( V, H );// 2.0 * dot( V, H ) * H - V;
		float NoL = saturate( dot( N, L ) );
		float NoH = saturate( dot( N, H ) );
		float VoH = saturate( dot( V, H ) );
		//L = normalize( vec3( ciViewMatrixInverse * vec4( L, 0.0 ) ) );
		
		//SpecularLighting += textureLod( uCubeMapTex, L, 0.0 ).rgb;
		
		if( NoL > 0 ) {
			float lod	= 0.0;
			vec3 lookup = L;//fix_cube_lookup( L, 128, lod );
			vec3 SampleColor = texture( uCubeMapTex, lookup, lod ).rgb;
			//vec3 SampleColor = pow( texture( uCubeMapTex, L ).rgb, vec3(5.0) ) * 4.0;
			float G = getGeometricShadowing( Roughness, NoV, NoL );
			vec3 F	= getFresnel( SpecularColor, VoH );
			//float D = getNormalDistribution( Roughness, NoH );
			SpecularLighting += SampleColor * F * G * 4 * VoH * (NoL / NoH);
		}
	}
//	return pow( SpecularLighting / float(NumSamples) * 0.1, vec3(3.0) );
	return SpecularLighting / float(NumSamples);
}


vec3 PrefilterEnvMap( ivec2 rand, float roughness, vec3 R )
{
	vec3 N = R;
	vec3 V = R;
	
	vec3 prefilteredColor = vec3(0);
	float  totalWeight      = 0;
	
	const int numSamples = 16;//8192*8;
	for(int i=0; i<numSamples; ++i)
	{
		vec2 xi  = Hammersley(i, numSamples,rand);
		vec3 H   = ImportanceSampleGGX(xi, roughness, N);
		vec3 L   = 2 * dot(V, H) * H - V;
		float  NoL = saturate(dot(N, L));
		if(NoL>0)
		{
			float lod	= 0.0;
			vec3 lookup = fix_cube_lookup( L, 128, lod );
			prefilteredColor += texture( uCubeMapTex, lookup, lod ).xyz * NoL;
			totalWeight += NoL;
		}
	}
	
	return prefilteredColor / totalWeight;
}


vec2 IntegrateBRDF( ivec2 Random, float Roughness, float NoV )
{
	vec3 V;
	V.x = sqrt( 1.0f - NoV * NoV );	// sin
	V.y = 0;
	V.z = NoV;						// cos
	
	float A = 0;
	float B = 0;
	
	const int NumSamples = 16;
	for( int i = 0; i < NumSamples; i++ )
	{
		vec2 E = Hammersley( i, NumSamples, Random );
		vec3 H = ImportanceSampleGGX( E, Roughness, vec3(0,0,1) );
		vec3 L = 2 * dot( V, H ) * H - V;
		
		float NoL = saturate( L.z );
		float NoH = saturate( H.z );
		float VoH = saturate( dot( V, H ) );
		
		if( NoL > 0 )
		{
			float G = G_Smith( Roughness, NoV, NoL );
			//float G_Vis = G * VoH / (NoH * NoV);
			float G_Vis = 4 * G * VoH * (NoL / NoH);
			
			float Fc = pow( 1 - VoH, 5 );
			A += (1 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}
	
	return vec2( A, B ) / NumSamples;
}

vec3 ApproximateSpecularIBL( ivec2 Random, vec3 SpecularColor, float Roughness, vec3 N, vec3 V )
{
	// Function replaced with prefiltered environment map sample
	vec3 R = 2 * dot( V, N ) * N - V;
	vec3 PrefilteredColor = PrefilterEnvMap( Random, Roughness, R );
	//vec3 PrefilteredColor = FilterEnvMap( Random, Roughness, N, V );
	
	// Function replaced with 2D texture sample
	float NoV = saturate( dot( N, V ) );
	vec2 AB = IntegrateBRDF( Random, Roughness, NoV );
	
	return PrefilteredColor * ( SpecularColor * AB.x + AB.y );
}

vec4 CosineSampleHemisphere( vec2 E )
{
	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt( E.y );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );
	vec3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;
	float PDF = CosTheta / PI;
	return vec4( H, PDF );
}

vec3 DiffuseIBL( ivec2 Random, vec3 DiffuseColor, vec3 N )
{
	vec3 DiffuseLighting = vec3(0);
	const int NumSamples = 16;
	for( int i = 0; i < NumSamples; i++ )
	{
		vec2 E = Hammersley( i, NumSamples, Random );
		vec3 L = CosineSampleHemisphere( E ).xyz;
		float NoL = saturate( dot( N, L ) );
		if( NoL > 0 )
		{
			vec3 SampleColor = texture( uCubeMapTex, L, 0 ).rgb;
			// lambert = DiffuseColor * NoL / PI
			// pdf = NoL / PI
			DiffuseLighting += SampleColor * DiffuseColor;
		}
	}
	return DiffuseLighting / NumSamples;
}


vec3 hash( vec3 p )
{
	p = vec3( dot(p,vec3(127.1,311.7, 74.7)),
			 dot(p,vec3(269.5,183.3,246.1)),
			 dot(p,vec3(113.5,271.9,124.6)));
	
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec3 p )
{
	vec3 i = floor( p );
	vec3 f = fract( p );
	
	vec3 u = f*f*(3.0-2.0*f);
	
	return mix( mix( mix( dot( hash( i + vec3(0.0,0.0,0.0) ), f - vec3(0.0,0.0,0.0) ),
						 dot( hash( i + vec3(1.0,0.0,0.0) ), f - vec3(1.0,0.0,0.0) ), u.x),
					mix( dot( hash( i + vec3(0.0,1.0,0.0) ), f - vec3(0.0,1.0,0.0) ),
						dot( hash( i + vec3(1.0,1.0,0.0) ), f - vec3(1.0,1.0,0.0) ), u.x), u.y),
			   mix( mix( dot( hash( i + vec3(0.0,0.0,1.0) ), f - vec3(0.0,0.0,1.0) ),
						dot( hash( i + vec3(1.0,0.0,1.0) ), f - vec3(1.0,0.0,1.0) ), u.x),
				   mix( dot( hash( i + vec3(0.0,1.0,1.0) ), f - vec3(0.0,1.0,1.0) ),
					   dot( hash( i + vec3(1.0,1.0,1.0) ), f - vec3(1.0,1.0,1.0) ), u.x), u.y), u.z );
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

void main() {
	// get the normal, light, position and half vector normalized
	vec3 N                  = normalize( vNormal );
	/*vec3 L                  = normalize( vLightPosition - vPosition );
	vec3 V                  = normalize( -vPosition );
	vec3 H					= normalize(V + L);
	
	// get all the usefull dot products and clamp them between 0 and 1 just to be safe
	float NoL				= saturate( dot( N, L ) );
	float NoV				= saturate( dot( N, V ) );
	float VoH				= saturate( dot( V, H ) );
	float NoH				= saturate( dot( N, H ) );
	*/
	// deduce the specular color from the baseColor and how metallic the material is
	vec3 specularColor		= mix( vec3( 0.08 * uSpecular ), uBaseColor, uMetallic );
	
	/*// compute the brdf terms
	float distribution		= getNormalDistribution( uRoughness, NoH );
	vec3 fresnel			= getFresnel( specularColor, VoH );
	float geom				= getGeometricShadowing( uRoughness, NoV, NoL );

	// get the specular and diffuse and combine them
	vec3 diffuse			= getDiffuse( uBaseColor, uRoughness, NoV, NoL, VoH );
	vec3 specular			= NoL * ( distribution * fresnel * geom );
	vec3 color				= uLightColor * ( diffuse + specular );
	
	// get the light attenuation from its radius
	float attenuation		= getAttenuation( vLightPosition, vPosition, uLightRadius );
	color					*= attenuation;
	*/
	vec3 color;
	//vec3 diffuseIBL			= textureLod( uCubeMapTex, fix_cube_lookup( normalize(vWsNormal), 512 ), ComputeCubemapMipFromRoughness( uRoughness, 15.0f ) ).rgb;
	
	vec3 wsNormal			= normalize(vWsNormal);
	
	float n = ( noise( vWsPosition * 2.1 ) + 1.0 ) * 0.5;
	n += ( noise( vWsPosition * 5.1 ) + 1.0 ) * 0.25;
	n += ( noise( vWsPosition * 15.1 ) + 1.0 ) * 0.15;
	n = 1.0-saturate( pow( n, 55.0 ) * 3.5 );
	
	float lod				= 7.9;
	
	ivec2 rand				= ivec2( ( 1.0 + random( wsNormal.xz ) ) * 8192.0 * 4.0 );
	vec3 specularIBL		= SpecularIBL( specularColor, uRoughness, wsNormal, vEyePosition, rand );
//	specularIBL				= ApproximateSpecularIBL( rand, specularColor, uRoughness, wsNormal, vEyePosition );
	//vec3 diffuseIBL			= PrefilterEnvMap( 1.0, wsNormal, rand );//textureLod( uCubeMapTex, fix_cube_lookup( normalize(vWsNormal), 512.0f, lod ), lod ).rgb;
	//vec3 diffuseIBL			= DiffuseIBL( rand, uBaseColor, wsNormal );
	//vec3 diffuseIBL			= DiffuseIBL2( uBaseColor, uRoughness, wsNormal, vEyePosition, rand );
	//color					= mix( diffuseIBL, specularIBL, sqrt(sqrt(1.0-uRoughness)) );
	color					= ( specularIBL ) * saturate( dot( wsNormal, vEyePosition ) );
	//vec3 approx = EnvBRDFApprox( specularColor, uRoughness, dot( wsNormal, vEyePosition ) );
	//color					*= diffuseIBL;
	//color = approx;
	//color = pow( color, vec3( 1.5 ) );
	

	//color = vec3(n + uRoughness);
	// apply the tone-mapping
	color					= Uncharted2Tonemap( color * uExposure );
	//color					= saturate( color );
	
	// white balance
	const float whiteInputLevel = 20.0f;
	vec3 whiteScale			= 1.0f / Uncharted2Tonemap( vec3( whiteInputLevel ) );
	color					= color * whiteScale;
	
	// gamma correction
	color					= pow( color, vec3( 1.0f / uGamma ) );
	
	// output the fragment color
    oColor                  = vec4( color, 1.0 );
}