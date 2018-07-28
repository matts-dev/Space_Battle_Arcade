#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 normalVec;
out vec3 fragmentPosition;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f);
	fragmentPosition = vec3(model * vec4(position, 1.0f));
	//normalVec = aNormal;

	//this allows non-uniform scaling
	//inverse is an expensive operation, and should be done on CPU first, then passed as uniform.
	//inverse is done here for simplicity.
	//each fragment will have to undergo the inverse calculation, which will be quite expensive.
	normalVec = mat3(transpose(inverse(model))) * aNormal;
}