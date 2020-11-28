#version 330 core
out vec4 fragColor;

//uniforms can be accessed in any shader, not just the shader they're defined -- they're global
uniform vec4 globalColor;

void main()
{
	fragColor = globalColor;
}