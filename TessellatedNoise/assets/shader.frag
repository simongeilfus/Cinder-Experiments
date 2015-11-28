/* 
 Tessellation Shader from Philip Rideout
 
 "Triangle Tessellation with OpenGL 4.0"
 http://prideout.net/blog/?p=48 */

#version 400

uniform float       uTime;
uniform samplerCube uRadianceMap;
uniform samplerCube uIrradianceMap;
uniform float       uRadianceMapSize;
uniform float       uIrradianceMapSize;

in vec3             gPosition;
in vec3             gNormal;
in vec4             gNoises;

out vec4            oColor;


#define saturate(x) clamp(x, 0.0, 1.0)

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

// Filmic tonemapping from
// http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap( vec3 x )
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
    const vec3 lightPosition = vec3( 2.5, 3.2, 2.0 );
    
    // get the normal, light, position and half vector normalized
    vec3 N                  = normalize( gNormal );
    vec3 V                  = normalize( gPosition );
    float NoV               = saturate( dot( N, V ) );

    // material settings
    float roughness         = 0.02 + saturate( 1.0 - smoothstep( -0.5, 1.0, gNoises.x ) + smoothstep( 0.0, 1.0, gNoises.y ) * gNoises.w * 6.0 );
    //roughness               = saturate( roughness - smoothstep( -0.085, 0.1, gNoises.w ) );
    float metallic          = smoothstep( 0.125, -0.3, gNoises.w );
    vec3 baseColor          = pow( vec3( gNoises.x + gNoises.y + gNoises.w, 0.0, gNoises.y * 0.25 ) * 0.5, vec3( 2.5 ) );
    
    // deduce the diffuse and specular color from the baseColor and how metallic the material is
    vec3 diffuseColor       = baseColor - baseColor * metallic;
    vec3 specularColor      = mix( vec3( 0.08 ), baseColor, metallic );
    
    // sample the pre-filtered cubemap at the corresponding mipmap level
    int numMips             = 6;
    float mip               = numMips - 1 + log2( roughness );
    vec3 lookup             = -reflect( V, N );
    vec3 radiance           = pow( textureLod( uRadianceMap, fix_cube_lookup( lookup, uRadianceMapSize, mip ), mip ).rgb, vec3( 2.2 ) );
    vec3 irradiance         = pow( texture( uIrradianceMap, fix_cube_lookup( N, uIrradianceMapSize, 0 ) ).rgb, vec3( 2.2 ) );

    
    // get the approximate reflectance
    vec3 reflectance        = EnvBRDFApprox( specularColor, pow( roughness, 4.0 ), NoV ) * 3.0;
    
    // combine the specular IBL and the BRDF
    vec3 diffuse            = diffuseColor * irradiance;
    vec3 specular           = radiance * reflectance;
    vec3 color              = ( diffuse + specular );

    // cheap fake ao by using the displacement and fading the diffuse term
    float fakeAO            = smoothstep( 0.5, 1.5, gNoises.w * 2.0 + 1.0 );
    color                   *= fakeAO;
    
    // apply the tone-mapping
    color                   = Uncharted2Tonemap( color * 2.0f );
    color                   = color * ( 1.0f / Uncharted2Tonemap( vec3( 20.0f ) ) );
    color                   = pow( color, vec3( 1.0f / 2.2f ) );
    
    oColor                  = vec4( vec3( color ), 1.0 );
}
