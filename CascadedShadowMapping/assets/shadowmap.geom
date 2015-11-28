#version 410 core

layout(triangles, invocations=4) in;
layout(triangle_strip, max_vertices=3) out;

uniform mat4 	uCascadesViewMatrices[4];
uniform mat4 	uCascadesProjMatrices[4];

out float 		gLayer;
out vec3		vsPosition;

void main() {
	for( int i = 0; i < gl_in.length(); ++i ) {
		vec4 pos 	= ( uCascadesViewMatrices[gl_InvocationID] * gl_in[i].gl_Position );
		gl_Position	= uCascadesProjMatrices[gl_InvocationID] * pos;
		vsPosition 	= pos.xyz;
		gl_Layer 	= gl_InvocationID;
		gLayer 		= float(gl_InvocationID);
		EmitVertex();
	}
	EndPrimitive();
} 