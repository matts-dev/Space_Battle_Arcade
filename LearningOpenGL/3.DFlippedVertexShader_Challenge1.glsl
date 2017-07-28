#version 330 core

//this vertex shader is designed to flip the triangle!

layout (location = 0) in vec3 vertPosition;
layout (location = 1) in vec3 inColor;
out vec3 passedColor;

void main()
{
	gl_Position = vec4(vertPosition, 1.0f);
	gl_Position.y = gl_Position.y * -1.0f;
	passedColor = inColor;
}
