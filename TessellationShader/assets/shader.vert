/* 
 Tessellation Shader from Philip Rideout
 
 "Triangle Tessellation with OpenGL 4.0"
 http://prideout.net/blog/?p=48 */

#version 400

in vec4         ciPosition;
out vec3        vPosition;

void main()
{
    vPosition = ciPosition.xyz;
}
