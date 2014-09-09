#version 400
#extension GL_ARB_shading_language_include : enable

#include "/lighting.glsl"

out vec3        oColor;
in vec3         gFacetNormal;
in vec3         gPosition;
uniform mat4    ciModelViewProjection;
uniform mat4    ciModelView;

uniform vec3 uLightPosition;


void main()
{
    vec3 normal         = normalize(gFacetNormal);
    
    vec3 diffuse        = getDiffuse( normal, uLightPosition );
    vec3 specular       = getSpecular( gPosition, normal, uLightPosition, vec3(1.0), vec3( 1.0 ), 3000.0 );
    float attenuation   = getAttenuation( length( gPosition - uLightPosition ), 450.0f, 0.3f );
    
    vec3 color          = attenuation * ( diffuse + specular );
    
    oColor = color;
}
