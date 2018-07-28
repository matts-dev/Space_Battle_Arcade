#version 330 core

out vec4 fragmentColor;

uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 lightPosInWorldSpace;

in vec3 normalVec;
in vec3 fragmentPosition;

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


	vec3 objectResultColor = (ambient + diffuseFromAngle) * objectColor;
	fragmentColor = vec4(objectResultColor, 1.f);
}
