#version 150

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3     vColor[3];

out vec3    gColor;
out vec3    gTriDistance;


void main()
{
    // Output the 3 point position of each triangles with
    // the corresponding barycentric coordinates and colors
    
    gl_Position     = gl_in[0].gl_Position;
    gTriDistance    = vec3(1, 0, 0);
    gColor          = vColor[0];
    EmitVertex();

    gl_Position     = gl_in[1].gl_Position;
    gTriDistance    = vec3(0, 1, 0);
    gColor          = vColor[1];
    EmitVertex();
    
    gl_Position     = gl_in[2].gl_Position;
    gTriDistance    = vec3(0, 0, 1);
    gColor          = vColor[2];
    EmitVertex();
    
    EndPrimitive();
}