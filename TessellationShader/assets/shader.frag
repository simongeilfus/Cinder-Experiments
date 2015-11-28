/* 
 Tessellation Shader from Philip Rideout
 
 "Triangle Tessellation with OpenGL 4.0"
 http://prideout.net/blog/?p=48 */

#version 400

out vec4    oColor;
in vec3     gFacetNormal;
in vec3     gTriDistance;
in vec3     gPatchDistance;
in float    gPrimitive;

const vec3 lightPosition    = vec3( 0.5, 0.2, 1.0 );
const vec3 diffuseColor     = vec3( 1.0 );
const vec3 ambientColor     = vec3( 0.025 );

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2*d*d);
    return d;
}

void main()
{
    vec3 N      = normalize(gFacetNormal);
    vec3 L      = lightPosition;
    float NoL   = abs(dot(N, L));
    vec3 color  = ambientColor + NoL * diffuseColor;
    
    float d1    = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
    float d2    = min(min(gPatchDistance.x, gPatchDistance.y), gPatchDistance.z);
    color       = amplify(d1, 40, -0.5) * amplify(d2, 60, -0.5) * color;
    
    oColor      = vec4( color, 1.0 );
}
