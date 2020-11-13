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
				uniform vec3 objectTint				= vec3(1,1,1);

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;

				vec3 CalculatePointLighting(vec3 normal, vec3 toView, vec3 fragPosition)
				{ 
					vec3 diffuseTexture = objectTint * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 ambientLight = lightAmbientIntensity * diffuseTexture;	

					//POINT LIGHT
					vec3 toLight = normalize(lightPosition - fragPosition);
					vec3 diffuseLight = max(dot(toLight, normal), 0) * lightDiffuseIntensity * diffuseTexture;

					vec3 toReflection = reflect(-toLight, normal);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = lightSpecularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
					float distance = length(lightPosition - fragPosition);
					float attenuation = 1 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);
					
					//DIRECTIONAL LIGHT (probably should cache texture lookup, but this is a debug shader anyways)
					//vec3 dirDiffuse = max(dot(-directionalLightDir, normal),0) * directionalLightColor * vec3(texture(material.texture_specular0, interpTextCoords));
					vec3 dirDiffuse = max(dot(-directionalLightDir, normal),0) * directionalLightColor * diffuseTexture;
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

					//debug ambient
					vec3 diffuseTexture = objectTint * vec3(texture(material.texture_diffuse0, interpTextCoords));
					vec3 ambientLight = vec3(0.05f) * diffuseTexture;	
					lightContribution += ambientLight;

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

			const char* const spaceModelShader_forward_vs = R"(
				#version 330 core
				layout (location = 0) in vec3 position;			
				layout (location = 1) in vec3 normal;	
				layout (location = 2) in vec2 textureCoordinates;
				layout (location = 3) in vec3 tangent;
				layout (location = 4) in vec3 bitangent;
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 fragNormal;
				out vec3 fragPosition;
				out vec2 interpTextCoords;
				out vec3 localPosition;
				out VS_OUT {
					mat3 TBN; //todo refactor this to just be a normal out (unless it can't because matrices are special?)
				} vert_out;

				void main(){
					gl_Position = projection * view * model * vec4(position, 1);
					fragPosition = vec3(model * vec4(position, 1));
					localPosition = position;

					//probably should calculate the inverse_tranpose matrix on CPU, it's a very costly operation
					mat3 normalMatrix = mat3(transpose(inverse(model)));

					fragNormal = normalize(normalMatrix * normal); //must normalize before interpolation! Otherwise models will be too bright!
					vec3 T = normalize(normalMatrix * tangent);
					vec3 B = normalize(normalMatrix * bitangent);
					vert_out.TBN = mat3(T, B, fragNormal);

					interpTextCoords = textureCoordinates;
				}
			)";


			const char* const spaceModelShader_forward_fs = R"(
				#version 330 core

				out vec4 fragmentColor;
				
				struct Material {
					sampler2D texture_diffuse0;   
					sampler2D texture_specular0;   
					sampler2D texture_normalmap0;	
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
				uniform vec3 objectTint				= vec3(1,1,1);

				uniform bool bUseNormalMapping		= true;
				uniform int renderMode				= 1;
				uniform bool bUseMirrorUvNormalCorrection = true;

				//one method of fixing UV seams is to modify the normal map around the seam, but this is hard when models don't have UV's set up so that the normal map seam is on one edge
				//so, I'm doing this work in the shader, dampening the effect of certain aspects of the normal map so that the seam is removed
				uniform bool bUseNormalSeamCorrection				= false;
				uniform float normalSeamCorrectionRange_LocalSpace	= 0.5f; //assuming all models are mirrored along same axis

				struct DirectionLight
				{
					vec3 dir_n;
					vec3 intensity;
				};	
				#define MAX_DIR_LIGHTS 4
				uniform DirectionLight dirLights[MAX_DIR_LIGHTS];

				in vec3 fragNormal;
				in vec3 fragPosition;
				in vec2 interpTextCoords;
				in vec3 localPosition;
				in VS_OUT {
					mat3 TBN; //#todo a little inconsistent with using this style out output and not above, but I prefer above for more direct in code? perhaps refactor.
				} vert_in;

				vec3 CalculatePointLighting(vec3 normal, vec3 toView, vec3 fragPosition)
				{ 
					vec3 diffuseTexture = objectTint * vec3(texture(material.texture_diffuse0, interpTextCoords));

					vec3 ambientLight = lightAmbientIntensity * diffuseTexture;	

					//POINT LIGHT
					vec3 toLight = normalize(lightPosition - fragPosition);
					vec3 diffuseLight = max(dot(toLight, normal), 0) * lightDiffuseIntensity * diffuseTexture;

					vec3 toReflection = reflect(-toLight, normal);
					float specularAmount = pow(max(dot(toView, toReflection), 0), material.shininess);
					vec3 specularLight = lightSpecularIntensity* specularAmount * vec3(texture(material.texture_specular0, interpTextCoords));
					
					float distance = length(lightPosition - fragPosition);
					float attenuation = 1 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);

					vec3 lightContribution = (ambientLight + diffuseLight + specularLight) * vec3(attenuation);
					
					//DIRECTIONAL LIGHT 
					vec3 dirDiffuse = vec3(0.f);
					vec3 dirSpecular = vec3(0.f);
					for(int light = 0; light < MAX_DIR_LIGHTS; ++light)
					{
						float n_dot_l = max(dot(normal, -dirLights[light].dir_n), 0.f);
						dirDiffuse += diffuseTexture.rgb * n_dot_l * dirLights[light].intensity;

						//specular is a little ad-hoc since I do not currently define a specular component of the light for stars.
						vec3 toSunLight_n = normalize(-dirLights[light].dir_n);
						float dirLightSpecAmount = pow(max(dot(toView, reflect(-toSunLight_n, normal)), 0), material.shininess);
						dirSpecular +=  dirLights[light].intensity * dirLightSpecAmount* vec3(texture(material.texture_specular0, interpTextCoords));
					}
					vec3 dirMapContrib = dirDiffuse + dirSpecular;
					#define TONE_MAP_DIR 1
					#if TONE_MAP_DIR
						dirMapContrib = (dirMapContrib) / (1 + dirMapContrib);
					#endif
					lightContribution += (dirMapContrib);

					//vec3 lightContribution = vec3(attenuation);	//uncomment to visualize attenuation
					return lightContribution;
				}

				void main(){
					vec3 normal;
					if(bUseNormalMapping)
					{
						//#TODO can I separate the TBN portion and put it in fragment shader but still do corrected normal here?
						normal = normalize(texture(material.texture_normalmap0, interpTextCoords).rgb); //unfortunately have to normalize what is stored in texture map
						normal = 2.0f * normal - 1.0f;
						normal = normalize(normal);

						//normalizing the matrix components since they've been interpolated.
						mat3 TBN_CORRECT = vert_in.TBN;

						//check if drawing a mirrored fragment
						vec3 tangent = TBN_CORRECT[0];
						vec3 bitangent = TBN_CORRECT[1];
						vec3 calcN = cross(tangent, bitangent);
						float normalsAligned = dot(calcN, TBN_CORRECT[2]);

						if(normalsAligned < 0 && bUseMirrorUvNormalCorrection){
							//flip handedness for mirrored texture coordinates
							TBN_CORRECT[0] = -TBN_CORRECT[0];

							normal = normalize(TBN_CORRECT * normal);
						} else {
							normal = normalize(TBN_CORRECT * normal);
						}

						//this helps hide the seams created by normal maps, though it does not look perfect.
						if(bUseNormalSeamCorrection)
						{
							if(abs(localPosition.x) < normalSeamCorrectionRange_LocalSpace)
							{
								float bufferMin = 0.0f; //for some region, apply dampening
								float dampenStrength = 1.f - clamp(abs(localPosition.x) / normalSeamCorrectionRange_LocalSpace, bufferMin, 1.f);
								normal.r = normal.r * (1-dampenStrength);
								normal = normalize(normal);
							}
						}
					} else {
						normal = normalize(fragNormal);
					}

											
					vec3 toView = normalize(cameraPosition - fragPosition);
					vec3 lightContribution = vec3(0.f);

					lightContribution += CalculatePointLighting(normal, toView, fragPosition);

					//debug ambient
					vec3 diffuseTexture = objectTint * vec3(texture(material.texture_diffuse0, interpTextCoords));
					vec3 ambientLight = vec3(0.05f) * diffuseTexture;	
					lightContribution += ambientLight;

					fragmentColor = vec4(lightContribution, 1.0f);

					//debug color changes
					if(renderMode > 0)
					{
						fragmentColor = vec4(normal,1.f);
						if(renderMode == 1)
						{
							//don't zero out anything
						}
						else if (renderMode == 2)
						{
							fragmentColor.g = fragmentColor.b = 0; //view red
						}
						else if (renderMode == 3)
						{
							fragmentColor.r = fragmentColor.b = 0;//view green
						}
						else if (renderMode == 4)
						{
							fragmentColor.r = fragmentColor.g = 0;//view blue
						}
					}
				}
			)";

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// debug normals
			const char* const normalDebugShader_LineEmitter_vs = R"(
				#version 330 core
				layout (location = 0) in vec3 position;			
				layout (location = 1) in vec3 normal;	
				layout (location = 2) in vec2 textureCoordinates;
				layout (location = 3) in vec3 tangent;
				layout (location = 4) in vec3 bitangent;
				
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out INTERFACE_BLOCK_VSOUT {
					vec3 vertNormal;
					mat3 TBN;
					vec2 uv;
				} vs_out;

				void main(){
					gl_Position = vec4(position, 1);

					//probably should calculate the inverse_tranpose matrix on CPU, it's a very costly operation
					mat3 normalMatrix = mat3(transpose(inverse(model)));

					vec3 N = normalize(normalMatrix * normal); //must normalize before interpolation! Otherwise models will be too bright!
					vec3 T = normalize(normalMatrix * tangent);
					vec3 B = normalize(normalMatrix * bitangent);

					vs_out.TBN = mat3(T, B, N);

					//model uv mirror correction
					vec3 calcN = cross(T, B);
					float normalsAligned = dot(calcN, vs_out.TBN[2]); 
					if(normalsAligned < 0){
						//flip handedness for mirrored texture coordinates
						vs_out.TBN[0] = -T; //doing just this doesn't look right, I think we need to flip T/B

						//vs_out.TBN[0] = B; //flip B and T with your right hand and you will see the normal points in correct direction
						//vs_out.TBN[1] = T; //flip B and T with your right hand and you will see the normal points in correct direction
						//vs_out.TBN[2] = -normalize(calcN);
					}

					vs_out.vertNormal = normal;
					vs_out.uv = textureCoordinates;
				}
			)";


			const char* const normalDebugShader_LineEmitter_gs = R"(
				#version 330 core
		
				layout (triangles) in;
				layout (line_strip, max_vertices=2) out;				

				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;
				uniform float normal_display_length = 1.f;
				uniform bool bUseNormalMapping = true;

				struct Material {
					sampler2D texture_diffuse0;   
					sampler2D texture_specular0;   
					sampler2D texture_normalmap0;	
					int shininess; /*32 is good default, but cannot default struct members in glsl*/
				};
				uniform Material material;

				in INTERFACE_BLOCK_VSOUT {
					vec3 vertNormal;
					mat3 TBN;
					vec2 uv;
				} vertices[];

				void main(){
					mat4 projection_view = projection*view;

					for(int i = 0; i < 3; ++i) 
					{
						vec3 normal = vertices[i].vertNormal;
						if(bUseNormalMapping)
						{
							//read the normal
							normal = normalize(texture(material.texture_normalmap0, vertices[i].uv).rgb); //unfortunately have to normalize what is stored in texture map
							normal = 2.0f * normal - 1.0f;
							normal = normalize(normal);

							//convert normal from TBN space to world space
							normal = normalize(vertices[i].TBN * normal);

						} else {
							normal = normalize(vertices[i].vertNormal);
						}

						vec4 vert = model * gl_in[i].gl_Position;
						vec4 tailPoint = vert + vec4((normal * normal_display_length), 0.f);

						gl_Position = projection_view * vert;	
						EmitVertex();

						gl_Position = projection_view * tailPoint;
						EmitVertex();

						EndPrimitive();
					}
										
				}
			)";

			const char* const normalDebugShader_LineEmitter_fs = R"(
				#version 330 core
				
				out vec4 fragmentColor;
						
				void main(){
					fragmentColor = vec4(1, 1, 0, 1);
				}
			)";
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			const char* const modelVertexOffsetShader_vs = R"(
				#version 330 core
				layout (location = 0) in vec3 position;			
				layout (location = 1) in vec3 normal;	
				layout (location = 2) in vec2 textureCoordinates;
				
				uniform float vertNormalOffsetScalar = 1.0f;
				uniform mat4 model;
				uniform mat4 view;
				uniform mat4 projection;

				out vec3 fragNormal;
				out vec3 fragPosition;
				out vec2 interpTextCoords;

				void main(){
					fragNormal = normalize(mat3(transpose(inverse(model))) * normal); //must normalize before interpolation! Otherwise low-scaled models will be too bright!
					interpTextCoords = textureCoordinates;

					vec3 normalOffset = normal*vertNormalOffsetScalar;
					gl_Position = projection * view * model  * vec4(position + normalOffset, 1);
					fragPosition = vec3(model * vec4(position + normalOffset, 1));
				}
			)";
			const char* const fwdModelHighlightShader_fs = R"(
				#version 330 core

				out vec4 fragmentColor;

				uniform vec3 color = vec3(1.f,1.f,1.f);

				void main(){
	
					vec3 lightContribution = color;

					#define TONE_MAP_FINAL_LIGHT 0
					#if TONE_MAP_FINAL_LIGHT
						lightContribution = (lightContribution) / (1 + lightContribution);
					#endif

					fragmentColor = vec4(lightContribution, 1.0f);
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


	char const* const spriteVS_src= R"(
				#version 330 core
				layout (location = 0) in vec3 position;				
				layout (location = 1) in vec2 inTexCoord;		
				
				out vec2 texCoord;

				uniform mat4 model;
				uniform mat4 projection;

				void main(){
					texCoord = inTexCoord;
					gl_Position = projection * model * vec4(position, 1.f);
				}
			)";
	char const* const spriteFS_src = R"(
				#version 330 core

				out vec4 fragmentColor;

				in vec2 texCoord;

				uniform sampler2D textureData;   

				void main(){
					//fragmentColor = vec4(1.0f, 1.f, 1.f, 1.f);//debug

					fragmentColor = texture(textureData, texCoord);
					if(fragmentColor.a < 0.05)
					{
						discard;
					}
				}
			)";

}