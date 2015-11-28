#version 400

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform mat4    ciViewMatrixInverse;
uniform mat3    ciNormalMatrix;

in vec3         teVsPosition[3];
in vec4         teNoises[3];
out vec3        gPosition;
out vec3        gNormal;
out vec4        gNoises;

void main()
{
    vec3 ab = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 ac = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 N = -vec3( ciViewMatrixInverse * vec4( normalize( cross( ab, ac ) ), 0.0 ) );

    
    for( int i = 0; i < gl_in.length(); ++i ) {
        gNormal     = N;
        gNoises     = teNoises[i];
        gPosition   = teVsPosition[i];
        gl_Position = gl_in[i].gl_Position; 
        EmitVertex();
    }
    
    EndPrimitive();
}