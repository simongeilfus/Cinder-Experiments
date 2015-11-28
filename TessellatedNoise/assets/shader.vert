#version 400

in vec4		ciPosition;
out vec3	vPosition;

void main()
{
    vPosition = ciPosition.xyz;
}
