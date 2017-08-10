#version 330 core

in vec3 color;
in vec2 texCoords;

out vec4 fragColor;

//uniform sampler2D textureData;
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	//rather than passing in different texture coordinates,  just use the relative ratio between them (I know the second image will be 2x as large)
	fragColor = mix(texture(texture1, texCoords / 2), texture(texture2, texCoords), 0.5f);

}
