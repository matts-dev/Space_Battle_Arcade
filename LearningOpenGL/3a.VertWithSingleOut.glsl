#version 330 core

layout (location = 0) in vec3 position;
out vec4 vertexSetColor;

void main()
{
	vertexSetColor = vec4(0.3f, 0.0f, 0.0f, 1.0f);
	gl_Position = vec4(position, 1.0f);
}
