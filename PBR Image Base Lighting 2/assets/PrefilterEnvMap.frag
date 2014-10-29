#version 150

uniform samplerCube uCubeMapTex;
uniform float		uLod;
uniform float		uMaxLod;
uniform float		uSize;

in vec3	NormalWorldSpace;

out vec3 	oColor;

#define saturate(x) clamp(x, 0.0, 1.0)
#define PI 3.1415926535897932384626433832795

// Interesting page on Hammersley Points
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html#
vec2 Hammersley(const in int index, const in int numSamples ){
	int reversedIndex = index;
	reversedIndex = (reversedIndex << 16) | (reversedIndex >> 16);
	reversedIndex = ((reversedIndex & 0x00ff00ff) << 8) | ((reversedIndex & 0xff00ff00) >> 8);
	reversedIndex = ((reversedIndex & 0x0f0f0f0f) << 4) | ((reversedIndex & 0xf0f0f0f0) >> 4);
	reversedIndex = ((reversedIndex & 0x33333333) << 2) | ((reversedIndex & 0xcccccccc) >> 2);
	reversedIndex = ((reversedIndex & 0x55555555) << 1) | ((reversedIndex & 0xaaaaaaaa) >> 1);
	
	return vec2(fract(float(index) / numSamples), float(reversedIndex) * 2.3283064365386963e-10);
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


// http://the-witness.net/news/2012/02/seamless-cube-map-filtering/
vec3 fix_cube_lookup( vec3 v, float cube_size, float lod ) {
	float M = max(max(abs(v.x), abs(v.y)), abs(v.z));
	float scale = 1 - exp2(lod) / cube_size;
	if (abs(v.x) != M) v.x *= scale;
	if (abs(v.y) != M) v.y *= scale;
	if (abs(v.z) != M) v.z *= scale;
	return v;
}


vec3 PrefilterEnvMap( float roughness, vec3 R )
{
	vec3 N = R;
	vec3 V = R;
	
	vec3 prefilteredColor = vec3(0);
	float  totalWeight      = 0;
	
	int numSamples = 8192 / int( uMaxLod - uLod );
	for(int i=0; i<numSamples; ++i)
	{
		vec2 xi  = Hammersley(i, numSamples);
		vec3 H   = ImportanceSampleGGX(xi, roughness, N);
		vec3 L   = 2 * dot(V, H) * H - V;
		float  NoL = saturate(dot(N, L));
		if(NoL>0)
		{
			vec3 lookup = fix_cube_lookup( L, uSize, uLod );
			prefilteredColor += textureLod( uCubeMapTex, lookup, 0 ).xyz * NoL;
			totalWeight += NoL;
		}
	}
	
	return prefilteredColor / totalWeight;
}

void main( void )
{
	vec3 color = texture( uCubeMapTex, NormalWorldSpace ).rgb;
	float roughness = uLod / uMaxLod;
	color = PrefilterEnvMap( pow( roughness, 6.0f ), NormalWorldSpace );
	oColor = color;
}