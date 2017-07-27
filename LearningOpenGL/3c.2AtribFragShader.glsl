#version 330 core

in vec3 passedColor;
out vec4 fragmentColor;

void main()
{
	fragmentColor = vec4(passedColor, 1.0f);
}
