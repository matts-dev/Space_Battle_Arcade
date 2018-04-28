#version 330 core

out vec4 fragColor;

// Turns out you can ignore incoming and uniforms and the shader will still compile.
//in vec2 texCoords;
//uniform sampler2D texture1;
//uniform sampler2D texture2;
//uniform float mixRatio;

void main()
{
	//fragColor = vec4(244.0/255, 140.0/255, 66.0/255, 1.0);
	fragColor = vec4(0.9568, 0.5490, 0.2588, 1.0);
}
