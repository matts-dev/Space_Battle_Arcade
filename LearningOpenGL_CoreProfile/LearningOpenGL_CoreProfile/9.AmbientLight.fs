#version 330 core

out vec4 fragmentColor;

uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
	float ambientStrength = 0.1f;
	vec3 ambient = ambientStrength * lightColor;

	vec3 objectResultColor = ambient * objectColor;
	fragmentColor = vec4(objectResultColor, 1.f);
}
