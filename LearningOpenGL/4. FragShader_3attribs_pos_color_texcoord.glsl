#version 330 core

in vec3 color;
in vec2 texCoords;

out vec4 fragColor;

//uniform sampler2D textureData;
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	//fragColor = texture(textureData, texCoords);
	//fragColor = texture(textureData, texCoords) * vec4(color, 1.0f);
	fragColor = mix(texture(texture1, texCoords), texture(texture2, texCoords), 0.5f);

}
