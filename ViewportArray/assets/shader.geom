// instanced geometry shader
// invocated once for each of the 4 viewports
// takes care of setting the viewport and transforming the vertices/primitives

#version 410 core

layout(triangles, invocations=4) in;
layout(triangle_strip, max_vertices=3) out;

uniform mat4 uMatrices[4];

in vec3 vColor[];
out vec3 gColor;
void main() {
	for( int i = 0; i < gl_in.length(); ++i ) {
		// transform the position here instead of in the vertex shader
		gl_Position			= uMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		// set the current viewport for this vertex
		gl_ViewportIndex	= gl_InvocationID + 1;
		gColor				= vColor[i];
		EmitVertex();
	}
	EndPrimitive();
} 