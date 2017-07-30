#version 330 core
layout (location = 0) in vec3 pos;

out vec4 vertexSetColor;

void main()
{
	gl_Position = vec4(pos, 1.0f);
	vertexSetColor = gl_Position;
}
