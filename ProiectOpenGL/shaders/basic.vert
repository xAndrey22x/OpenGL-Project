#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;

out vec3 fragPosEye;
out vec3 viewPosEye;
out vec3 lightPosEye[12];

out vec4 fragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 lightSpaceTrMatrix;

uniform vec3 pointLight[12];


void main() 
{
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);
	fPosition = vPosition;
	fNormal = vNormal;
	fTexCoords = vTexCoords;
	
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
	
	fragPosEye = fPosEye.xyz;
	
	for (int i = 0; i < 12; i++)
		lightPosEye[i] = vec3(view * vec4(pointLight[i], 1.0f));

	fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f); 
}
