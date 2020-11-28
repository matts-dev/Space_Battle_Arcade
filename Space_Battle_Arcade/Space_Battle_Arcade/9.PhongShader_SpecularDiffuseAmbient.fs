#version 330 core

out vec4 fragmentColor;

uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 lightPosInWorldSpace;
uniform vec3 viewerPos;

in vec3 normalVec;
in vec3 fragmentPosition;

const float specularIntensity = 0.5f;
const float shininessPower = 32;
//const float shininessPower = 256;

void main()
{
	//AMBIENT
	float ambientStrength = 0.1f;
	vec3 ambient = ambientStrength * lightColor;

	//DIFFUSE 
	vec3 norm = normalize(normalVec);
	vec3 lightDirection = normalize(lightPosInWorldSpace - fragmentPosition);
	float diffuseFromAngle = dot(norm, lightDirection); //take dot between the two unit vectors to get angle, which is the amount of diffuse 
	diffuseFromAngle = max(diffuseFromAngle, 0.0f); //correct for negative diffuse/angles
	vec3 diffuseColor = diffuseFromAngle * lightColor;

	//SPECULAR
	//const float specularIntensity = 0.5f;	 //in glsl you can define constants outside of main scope (see above)
	vec3 viewDirection = normalize(viewerPos - fragmentPosition);
	vec3 reflectionDir = reflect(-lightDirection, norm); //expecting the vector to be pointing towards fragment position 
	float rawSpecular = dot(reflectionDir, viewDirection); //both are normals, so we get angle (ie cos(theta))
	float clampedSpecular = max(rawSpecular, 0.0f);
	float specFactor = pow(clampedSpecular, shininessPower); //this increaess brightness
	vec3 specular = specFactor * specularIntensity * lightColor; //specular favors the color of the light, not the object.

	vec3 objectResultColor = (specular + ambient + diffuseFromAngle) * objectColor;
	fragmentColor = vec4(objectResultColor, 1.f);
}
