#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Geometry Stage Shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* const gbufferShader_vs = R"(
	#version 330 core
	layout (location = 0) in vec3 position;			
	layout (location = 1) in vec3 normal;	
	layout (location = 2) in vec2 textureCoordinates;
				
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	out vec3 fragNormal;
	out vec3 fragPosition;
	out vec2 interpTextCoords;

	void main(){
		gl_Position = projection * view * model * vec4(position, 1);
		fragPosition = vec3(model * vec4(position, 1));

		fragNormal = normalize(mat3(transpose(inverse(model))) * normal); //must normalize before interpolation! Otherwise low-scaled models will be too bright!

		interpTextCoords = textureCoordinates;
	}
)";
const char* const gbufferShader_fs = R"(
	#version 330 core

	layout (location = 0) out vec3 position;
	layout (location = 1) out vec3 normal;
	layout (location = 2) out vec4 albedo_spec;
				
	struct Material {
		sampler2D texture_diffuse0;
		sampler2D texture_specular0;
		int shininess;
	};
	uniform Material material;			

	uniform vec3 cameraPosition;

	in vec3 fragNormal;
	in vec3 fragPosition;
	in vec2 interpTextCoords;

	void main(){
		position.rgb = fragPosition;
		normal.rgb = fragNormal;
		albedo_spec.rgb = texture(material.texture_diffuse0, interpTextCoords).rgb;
		albedo_spec.a = texture(material.texture_specular0, interpTextCoords).r;
	}
	)";


const char* const gbufferShader_emissive_fs = R"(
	#version 330 core

	//framebuffer locations 
	layout (location = 0) out vec3 position;
	layout (location = 1) out vec3 normal;
	layout (location = 2) out vec4 albedo_spec;

	uniform vec3 cameraPosition;
	uniform vec3 lightColor	= vec3(1, 1, 1);

	in vec3 fragNormal;
	in vec3 fragPosition;
	in vec2 interpTextCoords;

	void main(){
		position.rgb = fragPosition;
		normal.rgb = fragNormal;
		albedo_spec.rgb = lightColor;
		albedo_spec.a = 1.f;
	}
)";
