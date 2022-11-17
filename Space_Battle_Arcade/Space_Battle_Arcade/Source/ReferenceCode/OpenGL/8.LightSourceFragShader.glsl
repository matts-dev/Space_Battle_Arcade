#version 330 core

out vec4 fragmentColor;

uniform vec3 lightColor;

void main()
{
	//the sole purpose of this shader is to make the light emiting cube have a different appearance
	fragmentColor = vec4(lightColor, 1.f);
}