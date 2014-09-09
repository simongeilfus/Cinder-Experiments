#version 400

uniform mat3    ciNormalMatrix;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 tePosition[3];
in vec3 teNormal[3];
out vec3 gFacetNormal;
out vec3 gPosition;


void main()
{
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    vec3 n = normalize(ciNormalMatrix * cross(A, B));
    gFacetNormal = n;
    
    gl_Position = gl_in[0].gl_Position;
    gPosition = gl_in[0].gl_Position.xyz;
    EmitVertex();
    
    gl_Position = gl_in[1].gl_Position;
    gPosition = gl_in[1].gl_Position.xyz;
    EmitVertex();
    
    gl_Position = gl_in[2].gl_Position;
    gPosition = gl_in[2].gl_Position.xyz;
    EmitVertex();
    
    EndPrimitive();
}