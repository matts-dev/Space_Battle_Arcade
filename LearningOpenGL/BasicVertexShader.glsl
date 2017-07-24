#version 330 core
layout (location = 0) in vec3 attrib0;

void main()
{
	gl_Position = vec4(attrib0, 1.0f);
}
