#version 330 core

layout (location = 0) in vec3 attribPosition; //attrib set position
layout (location = 1) in vec3 attribColor;	//attrib set color

out vec3 passedColor; //the output for color
void main()
{
	gl_Position = vec4(attribPosition, 1.0f);	
	passedColor = attribColor; //directly pass what was provided with the attribute
}
