#version 330 core

layout (location = 0) in vec3 position;
uniform vec4 offset;

void main()
{
	gl_Position = vec4(position, 1.0f);
	gl_Position.x += offset.x;
	gl_Position.y += offset.y;
	gl_Position.z += offset.z;
}