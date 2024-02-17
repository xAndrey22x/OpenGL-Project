#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

in vec3 fragPosEye;
in vec3 viewPosEye;
in vec3 lightPosEye[12];

in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

uniform vec3 pointLight;
uniform vec3 pointLightColor;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform sampler2D shadowMap;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;

float constant = 1.0f;
float linear = 0.22f;
float quadratic = 0.20f;

vec3 ambientPoint;
vec3 diffusePoint;
vec3 specularPoint;

uniform float fogDensity;

void computeDirLight()
{
    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));

    //compute view direction
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute ambient light
    ambient = ambientStrength * lightColor;

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
}


void computePointLight(int i)
{
	vec3 normalEye = normalize(normalMatrix * fNormal);
	//compute view direction
	vec3 viewDirN = normalize(viewPosEye);
	//compute light direction
	vec3 lightDirN = normalize(lightPosEye[i] - fragPosEye.xyz);
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);
	//compute specular light
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), 32);
	specular = specularStrength * specCoeff * pointLightColor;
	//compute distance to light
	float dist = length(lightPosEye[i] - fragPosEye.xyz);
	//compute attenuation
	float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));
    ///compute ambient light
	ambientPoint = att * ambientStrength * pointLightColor;
	//compute diffuse light
	diffusePoint = att * max(dot(normalEye, lightDirN), 0.0f) * pointLightColor;
	specularPoint = att * specularStrength * specCoeff * pointLightColor;
}

float computeFog()
{
 float fragmentDistance = length(fragPosEye);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
 
 return clamp(fogFactor, 0.0f, 1.0f);
}

float computeShadow()
{
	// perform perspective divide
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// Transform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	// Get depth of current fragment from light's perspective
	float currentDepth = normalizedCoords.z;
	// Check whether current frag pos is in shadow
	float bias = 0.005f;
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
	if (normalizedCoords.z > 1.0f)
		return 0.0f;
	return shadow;
}


void main() 
{
    computeDirLight();
	vec3 totalAmbient = ambient;
	vec3 totalDiffuse = diffuse;
	vec3 totalSpecular = specular;
	
	for (int i = 0; i < 12; i++){
		computePointLight(i);
		totalAmbient += ambientPoint;
		totalDiffuse += diffusePoint;
		totalSpecular += specularPoint;
	}
	
	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	
	float shadow = computeShadow();

    // Compute final vertex color
    vec3 color = min((totalAmbient + (1.0f - shadow)*totalDiffuse) * texture(diffuseTexture, fTexCoords).rgb + ((1.0f - shadow) * totalSpecular) * texture(specularTexture, fTexCoords).rgb, 1.0f);

    fColor = mix(fogColor, vec4(color, 1.0f), fogFactor);

}
