#pragma once
namespace SA
{
	const char* const litObjectShader_VertSrc = R"(
				#version 330 core
				layout (location = 0) in vec3 position;			
				layout (location = 1) in vec3 normal;	
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 fragNormal;
				out vec3 fragPosition;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(model * vec4(position, 1));

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal);
				}
			)";
	const char* const litObjectShader_FragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor = vec3(1,1,1);
				uniform vec3 objectColor = vec3(1,1,1);
				uniform float ambientStrength = 0.1f; 
				uniform vec3 lightPosition;
				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;

				void main(){
					vec3 ambientLight = ambientStrength * lightColor;					

					vec3 toLight = normalize(lightPosition - fragPosition);
					vec3 normal = normalize(fragNormal);									//interpolation of different normalize will cause loss of unit length
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * lightColor;

					float specularStrength = 0.5f;
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 toReflection = reflect(-toView, normal);							//reflect expects vector from light position
					float specularAmount = pow(max(dot(toReflection, toLight), 0), 32);
					vec3 specularLight = specularStrength * lightColor * specularAmount;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * objectColor;
					
					fragmentColor = vec4(lightContribution, 1.0f);
				}
			)";

	const char* const forwardShadedModel_SimpleLighting_vertSrc = R"(
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

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal); //must normalize before interpolation! Otherwise low-scaled models will be too bright!

					interpTextCoords = textureCoordinates;
				}
			)";
	const char* const forwardShadedModel_SimpleLighting_fragSrc = R"(
				#version 330 core

				out vec4 fragmentColor;
				
				struct Material {
					sampler2D texture_diffuse0;   
					sampler2D texture_specular0;   
					int shininess; /*32 is good default, but cannot default struct members in glsl*/
				};
				uniform Material material;			
				uniform vec3 cameraPosition;

				uniform vec3 lightPosition			= vec3(0,0,0);
				uniform vec3 lightAmbientIntensity  = vec3(0.05f, 0.05f, 0.05f);
				uniform vec3 lightDiffuseIntensity  = vec3(0.8f, 0.8f, 0.8f);
				uniform vec3 lightSpecularIntensity = vec3(1.f, 1.f, 1.f); 
				uniform float lightConstant			= 1.f;
				uniform float lightLinear			= 0.09f;
				uniform float lightQuadratic		= 0.032f;
				uniform vec3 directionalLightDir	= vec3(1, -1, -1);
				uniform vec3 directionalLightColor	= vec3(1, 1, 1);

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;

				vec3 CalculatePointLighting(vec3 normal, vec3 toView, vec3 fragPosition)
				{ 
					vec3 ambientLight = lightAmbientIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));	

					//POINT LIGHT
					vec3 toLight = normalize(lightPosition - fragPosition);
					vec3 diffuseLight = max(dot(toLight, normal), 0) * lightDiffuseIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = lightSpecularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
					float distance = length(lightPosition - fragPosition);
					float attenuation = 1 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);
					
					//DIRECTIONAL LIGHT (probably should cache texture lookup, but this is a debug shader anyways)
					vec3 dirDiffuse = max(dot(-directionalLightDir, normal),0) * directionalLightColor * vec3(texture(material.texture_specular0, interpTextCoords));
					//vec3 dirDiffuse = max(dot(-directionalLightDir, normal),0) * directionalLightColor * vec3(texture(material.texture_diffuse0, interpTextCoords));
					vec3 toSunLight = normalize(-directionalLightDir);
					float dirLightSpecAmount = pow(max(dot(toView, reflect(-toSunLight, normal)), 0), material.shininess);
					vec3 dirSpecular =  directionalLightColor * dirLightSpecAmount* vec3(texture(material.texture_specular0, interpTextCoords));
					lightContribution += (dirDiffuse + dirSpecular);


					//vec3 lightContribution = vec3(attenuation);	//uncomment to visualize attenuation
					return lightContribution;
				}

				void main(){
					vec3 normal = normalize(fragNormal);														
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 lightContribution = vec3(0.f);

					lightContribution += CalculatePointLighting(normal, toView, fragPosition);

					fragmentColor = vec4(lightContribution, 1.0f);
				}
			)";

	const char* const forwardShadedModel_AmbientLight = R"(
				#version 330 core

				out vec4 fragmentColor;
				
				struct Material {
					sampler2D texture_diffuse0;   
					sampler2D texture_specular0;   
					int shininess; /*32 is good default, but cannot default struct members in glsl*/
				};
				uniform Material material;			
				uniform vec3 cameraPosition;
				uniform vec3 tint = vec3(1.0, 1.0, 1.0);

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;

				void main(){
					vec3 ambientLight = tint * vec3(texture(material.texture_diffuse0, interpTextCoords));	

					fragmentColor = vec4(ambientLight, 1.0f);
				}
			)";

	const char* const forwardShadedModel_Emissive_fragSrc = R"(
				#version 330 core

				out vec4 fragmentColor;

				uniform vec3 cameraPosition;
				uniform vec3 lightColor	= vec3(1, 1, 1);

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;

				void main(){
					fragmentColor = vec4(lightColor	, 1.0f);
				}
			)";


	const char* const forwardShadedModel_vertSrc = R"(
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

					//calculate the inverse_tranpose matrix on CPU in real applications; it's a very costly operation
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal); //must normalize before interpolation! Otherwise low-scaled models will be too bright!

					interpTextCoords = textureCoordinates;
				}
			)";
	const char* const forwardShadedModel_fragSrc = R"(
				#version 330 core

				out vec4 fragmentColor;
				
				struct Material {
					sampler2D texture_diffuse0;   
					sampler2D texture_specular0;   
					int shininess;
				};
				uniform Material material;			

				struct PointLight
				{
					vec3 position;

					//intensities are like strengths -- but they may contain color information.
					vec3 ambientIntensity;
					vec3 diffuseIntensity;
					vec3 specularIntensity;

					//attenuation constants
					float constant;
					float linear;
					float quadratic;
				};	
				#define NUM_PNT_LIGHTS 4
				uniform PointLight pointLights[NUM_PNT_LIGHTS];

				struct DirectionalLight
				{
					vec3 direction;
					//intensities are strengths, but may contain color information.
					vec3 ambientIntensity;
					vec3 diffuseIntensity;
					vec3 specularIntensity;
				};
				uniform DirectionalLight dirLight;

				struct ConeLight 
				{
					float cos_cutoff;
					float cos_outercutoff;
					vec3 position;
					vec3 direction;

					//intensities are strengths, but may contain color information.
					vec3 ambientIntensity;
					vec3 diffuseIntensity;
					vec3 specularIntensity;	
	
					//attenuation constants
					float constant;
					float linear;
					float quadratic;			
				};
				uniform ConeLight coneLight;

				uniform vec3 cameraPosition;

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;

				/* uniforms can have initial value in GLSL 1.20 and up */
				uniform int enableAmbient = 1;
				uniform int enableDiffuse = 1;
				uniform int enableSpecular = 1;
				uniform int toggleFlashLight = 0;
				
				//function forward declarations
				vec3 CalculateDirectionalLighting(DirectionalLight light, vec3 normal, vec3 toView);
				vec3 CalculateConeLighting(ConeLight light, vec3 normal, vec3 toView);
				vec3 CalculatePointLighting(PointLight light, vec3 normal, vec3 toView, vec3 fragPosition);

				void main(){
					vec3 normal = normalize(fragNormal);														
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 lightContribution = vec3(0.f);

					lightContribution += CalculateDirectionalLighting(dirLight, normal, toView);					
					for(int i = 0; i < NUM_PNT_LIGHTS; ++i)
					{
						lightContribution += CalculatePointLighting(pointLights[i], normal, toView, fragPosition);
					}
					lightContribution += CalculateConeLighting(coneLight, normal, toView) * vec3(toggleFlashLight);

					fragmentColor = vec4(lightContribution, 1.0f);
				}

				vec3 CalculateDirectionalLighting(DirectionalLight light, vec3 normal, vec3 toView)
				{
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));	

					vec3 toLight = normalize(-light.direction);
					vec3 diffuseLight = max(dot(toLight, normal), 0) * light.diffuseIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);												
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight);
					
					return lightContribution;
				}


				vec3 CalculatePointLighting(PointLight light, vec3 normal, vec3 toView, vec3 fragPosition)
				{ 
					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));	

					vec3 toLight = normalize(light.position - fragPosition);
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					float distance = length(light.position - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);
					//vec3 lightContribution = vec3(attenuation);	//uncomment to visualize attenuation

					return lightContribution;
				}
				vec3 CalculateConeLighting(ConeLight light, vec3 normal, vec3 toView)
				{ 
					vec3 toLight = normalize(light.position - fragPosition);
					float cosTheta = dot(-toLight, normalize(light.direction));
					
					//note light.cutoff will have a higher value than light.outercutoff because they're cos(angle) values; Smaller angles will be closer to 1.
					float epsilon = light.cos_cutoff - light.cos_outercutoff;
					float intensity = (cosTheta - light.cos_outercutoff) / epsilon;
					intensity = clamp(intensity, 0, 1);

					vec3 ambientLight = light.ambientIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));	
					vec3 diffuseLight = max(dot(toLight, fragNormal), 0) * light.diffuseIntensity * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 toReflection = reflect(-toLight, normal);							

					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = light.specularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
					ambientLight *= enableAmbient;
					diffuseLight *= enableDiffuse;
					specularLight *= enableSpecular;

					float distance = length(light.position - fragPosition);
					float attenuation = 1 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

					//allow ambient light regardless of spotlight cone
					vec3 lightContribution = ambientLight + (diffuseLight + specularLight) * intensity;
					lightContribution *= attenuation;					

					return lightContribution;
				}
			)";

	const char* const lightLocationShader_VertSrc = R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
				}
			)";
	const char* const lightLocationShader_FragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;

				uniform vec3 lightColor;
				
				void main(){
					fragmentColor = vec4(lightColor, 1.f);
				}
			)";

	char const* const DebugVertSrc = R"(
				#version 330 core
				layout (location = 0) in vec4 position;				
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				void main(){
					gl_Position = projection * view * model * position;
				}
			)";
	char const* const DebugFragSrc = R"(
				#version 330 core
				out vec4 fragmentColor;
				uniform vec3 color = vec3(1.f,1.f,1.f);
				void main(){
					fragmentColor = vec4(color, 1.f);
				}
			)";

}