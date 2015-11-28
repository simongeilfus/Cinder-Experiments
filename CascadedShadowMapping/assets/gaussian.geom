#version 410 core

layout(triangles, invocations=4) in;
layout(triangle_strip, max_vertices=3) out;

out float gLayer;

void main() {
	for( int i = 0; i < gl_in.length(); ++i ) {
		gl_Position	= gl_in[i].gl_Position;
		gl_Layer 	= gl_InvocationID;
		gLayer 		= float( gl_InvocationID );
		EmitVertex();
	}
	EndPrimitive();
} 