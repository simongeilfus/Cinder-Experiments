#version 150

uniform mat4	ciModelViewProjection;

in vec4			ciPosition;

out vec3		vDirection;

void main( void )
{
	vDirection 	= vec3( ciPosition );
	gl_Position = ciModelViewProjection * ciPosition;
}
