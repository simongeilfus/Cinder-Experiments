#version 410 core

uniform samplerCube	uCubeMap;
uniform vec3 		uCubeMapSize;
uniform vec3 		uCubeMapPosition;
uniform vec3		uCameraPosition;
uniform mat3 	ciNormalMatrix;

uniform sampler2D 	uRoughnessMap;
uniform sampler2D 	uLightMap;
uniform sampler2D 	uNormalMap;
uniform vec3 		uLight0Color;
uniform vec3 		uLight1Color;
uniform vec3 		uLight2Color;
uniform vec3 		uLight3Color;
uniform float 		uLight1Intensity;
uniform float 		uLight2Intensity;
uniform float 		uLight3Intensity;

uniform float 		uReflections;

in vec2				vUv;
in vec3				vPosition;
in vec3				vNormal;
in vec3				vReflection;

out vec4			oColor;

// Cubemap seams fixed lookup
// http://the-witness.net/news/2012/02/seamless-cube-map-filtering/
vec3 fix_cube_lookup( vec3 v, float cube_size, float lod ) {
	float M = max(max(abs(v.x), abs(v.y)), abs(v.z));
	float scale = 1.0 - exp2(lod) / cube_size;
	if (abs(v.x) != M) v.x *= scale;
	if (abs(v.y) != M) v.y *= scale;
	if (abs(v.z) != M) v.z *= scale;
	return v;
}

// Moving Frostbite to PBR
// http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
float computeDistanceBaseRoughness( float distInteresectionToShadedPoint, float distInteresectionToProbeCenter, float linearRoughness )
{
	// To avoid artifacts we clamp to the original linearRoughness
	// which introduces an acceptable bias and allows conservation
	// of mirror reflection behavior for a smooth surface .
	float newLinearRoughness = clamp ( distInteresectionToShadedPoint /
									  distInteresectionToProbeCenter * linearRoughness , 0.0, linearRoughness );
	return mix( newLinearRoughness , linearRoughness , linearRoughness );
}

// Parallax corrected cubemaps
// http://www.clicktorelease.com/blog/making-of-cruciform
// https://seblagarde.wordpress.com/2012/11/28/siggraph-2012-talk/
vec3 getBoxIntersection( vec3 pos, vec3 reflectionVector, vec3 cubeSize, vec3 cubePos )
{
	vec3 rbmax = (   0.5f * ( cubeSize - cubePos ) - pos ) / reflectionVector;
	vec3 rbmin = ( - 0.5f * ( cubeSize - cubePos ) - pos ) / reflectionVector;
	
	vec3 rbminmax = vec3(
		( reflectionVector.x > 0.0f ) ? rbmax.x : rbmin.x,
		( reflectionVector.y > 0.0f ) ? rbmax.y : rbmin.y,
		( reflectionVector.z > 0.0f ) ? rbmax.z : rbmin.z );
	
	float correction = min( min( rbminmax.x, rbminmax.y ), rbminmax.z );
	return ( pos + reflectionVector * correction );
}

// Normal Blending
// Source adapted from http://blog.selfshadow.com/publications/blending-in-detail/
vec3 blendNormals( vec3 baseNormal, vec3 detailsNormal )
{
    mat3 nBasis = mat3(
        vec3(baseNormal.z, baseNormal.y, -baseNormal.x), // +90 degree rotation around y axis
        vec3(baseNormal.x, baseNormal.z, -baseNormal.y), // -90 degree rotation around x axis
        vec3(baseNormal.x, baseNormal.y,  baseNormal.z));
    return normalize( detailsNormal.x*nBasis[0] + detailsNormal.y*nBasis[1] + detailsNormal.z*nBasis[2] );
}

void main() {
	vec2 uv 		= vec2( vUv.x, 1.0 + vUv.y );

	// sample the lighting contribution from the lightmap
	vec3 lighting 	= pow( texture( uLightMap, uv ).rgb, vec3( 2.2f ) ); 

	// calculate the reflection vector from the normal and the camera position
	vec3 V 			= normalize( vPosition - uCameraPosition );
	vec3 N 			= blendNormals( 
		normalize( vNormal ), 
		( texture( uNormalMap, uv ).xyz * 2.0f - 1.0f ) * vec3( 0.35f, 0.35f, 1.0f ) );
	vec3 R			= reflect( V, N ); 
	vec3 boxInters 	= getBoxIntersection( vPosition, R, uCubeMapSize, uCubeMapPosition );
	vec3 lookup 	= boxInters - uCubeMapPosition;
	
	// calculate the mip level from roughness
	float roughness = texture( uRoughnessMap, uv ).r;
	float distRough = computeDistanceBaseRoughness( 
		length( vPosition - boxInters ), 
		length( uCubeMapPosition - boxInters ), 
		0.09f + roughness );
	float numMips	= 7.0;
	float mip		= 1.0 - 1.2 * log2( distRough );
	mip				= numMips - 1.0 - mip;

	// fix the lookup vector
	lookup			= fix_cube_lookup( lookup, 1024.0, mip );

	// sample cubemap
	vec3 reflection	= pow( textureLod( uCubeMap, lookup, mip ).rgb, vec3( 2.2f ) ) * pow( 1.0 - roughness, 1.8f ); 
	
	// output the lighting and the reflection
    oColor 			= vec4( lighting + mix( vec3( 0.0f ), reflection, uReflections ), 1.0 );

    // gamma correction
    oColor.rgb 		= pow( oColor.rgb, vec3( 1.0 / 2.2 ) );
}