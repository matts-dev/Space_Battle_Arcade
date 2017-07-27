#version 330 core

in vec4 vertexSetColor;
out vec4 fragColor;

void main()
{
	fragColor = vertexSetColor;
}
