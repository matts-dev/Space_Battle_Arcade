#version 330 core

in vec2 texCoords;

//modify the output to write to framebuffer location 0 (this should be set to color_attachement on client side)
layout(location = 0) out vec4 fragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform float mixRatio;
void main()
{
	fragColor = mix(texture(texture1, texCoords), texture(texture2, texCoords), mixRatio);
}
