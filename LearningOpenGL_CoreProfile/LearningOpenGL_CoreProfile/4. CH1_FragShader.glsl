#version 330 core

in vec3 color;
in vec2 texCoords;
vec2 secondTexCoords;

out vec4 fragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	//TEXTURE COORDINATES ARE FROM 0 TO 1, not like viewport coordinates
	secondTexCoords = -texCoords;
	secondTexCoords.y = 1f - secondTexCoords.y ;

	fragColor = mix(texture(texture1, texCoords), texture(texture2, secondTexCoords), 0.5f);
}
