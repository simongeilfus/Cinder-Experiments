#version 400
#extension GL_ARB_shading_language_include : enable

#include "/lighting.glsl"

out vec3        oColor;
in vec3         gFacetNormal;
in vec3         gPosition;
uniform mat4    ciModelViewProjection;
uniform mat4    ciModelView;

uniform vec3    uLightPosition;

const vec3 diffuseColor     = vec3(1.0);
const vec3 specularColor    = vec3(1.0);

void main()
{
    vec3 normal         = normalize(gFacetNormal);
    vec3 diffuse        = diffuseColor * getDiffuse( normal, uLightPosition );
    vec3 specular       = specularColor * getSpecular( gPosition, normal, uLightPosition, 0.35f ) * 0.25f;
    float attenuation   = getAttenuation( length( gPosition - uLightPosition ), 450.0f, 0.13f );
    oColor              = attenuation * vec3( diffuse + specular );
}
