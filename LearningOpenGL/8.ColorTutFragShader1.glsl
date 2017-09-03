#version 330 core

out vec4 fragmentColor;

uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
	//fragmentColor = vec4(1.0f, 1.f, 1.f, 1.f);
	fragmentColor = vec4(lightColor * objectColor, 1.f);
}