#version 330 core

in vec3 color;
in vec2 texCoords;

out vec4 fragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform float mixRatio;
void main()
{
	fragColor = mix(texture(texture1, texCoords), texture(texture2, texCoords), mixRatio);
}
