#version 150

uniform vec3        uLightPosition;
uniform samplerCube uDepthMap;

in vec3             vNormal;
in vec3             vPosition;
in vec3             vViewSpaceLightPosition;
in vec3             vViewSpacePosition;
out vec4            oColor;

void main() {
    vec3 positionToLight    = vPosition - uLightPosition;

    const float eps         = 0.01;
    float depth             = texture( uDepthMap, positionToLight ).b;
    float shadow            = float( depth + eps > length( positionToLight ) );

    vec3 n                  = normalize( vNormal );
    vec3 l                  = normalize( vViewSpaceLightPosition - vViewSpacePosition );
    float diffuse           = max( dot( n, l ), 0.0 );
    oColor                  = vec4( shadow * diffuse );
}