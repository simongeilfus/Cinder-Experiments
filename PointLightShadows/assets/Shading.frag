#version 150

uniform vec3        uLightPosition;
uniform samplerCube uDepthMap;

//in vec3             vNormal;
in vec3             vPosition;
out vec4            oColor;

void main() {
    vec3 positionToLight    = vPosition - uLightPosition;

    const float eps         = 0.01;
    float depth             = texture( uDepthMap, positionToLight ).b;
    float shadow            = float( depth + eps > length( positionToLight ) );

    oColor                  = vec4( shadow );
}