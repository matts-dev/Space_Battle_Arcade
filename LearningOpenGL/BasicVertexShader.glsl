#version 330 core
layout (location = 0) in vec3 attrib0;

void main()
{
	gl_position = vec4(attrib0, 0.0f);
}

