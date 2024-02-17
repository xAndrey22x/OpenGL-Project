#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"


#include <iostream>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// Point Lights
glm::vec3 pointLight[12];
glm::vec3 pointLightColor;

GLint pointLightLoc;
GLint pointLightColorLoc;

// Fog
GLint fogDensityLoc;
GLfloat fogDensity;

// camera
gps::Camera myCamera(
	glm::vec3(75.0f, 11.6f, -0.2f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.3f;

GLboolean pressedKeys[1024];

float yaw = 0.0f; // Initial yaw angle
float pitch = 0.0f; // Initial pitch angle
float lastX = 0.0f;
float lastY = 0.0f;
bool firstMouse = true;

// models
gps::Model3D cartier;
gps::Model3D dodge;
gps::Model3D eliceZ;
gps::Model3D eliceY;
gps::Model3D rain;
GLfloat angle;

// shaders
gps::Shader myBasicShader;
gps::Shader skyboxShader;
gps::Shader depthMapShader;

// SkyBox
gps::SkyBox mySkyBox;
std::vector<const GLchar*> faces;
std::vector<const GLchar*> faces2;

// Animation
bool automaticAnimationInProgress = true;
float animationStartTime;
float animationDuration = 6.5f;  // Intro animation time

//Shadow
GLuint shadowMapFBO;
GLuint depthMapTexture;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
glm::mat4 lightRotation;
GLfloat lightAngle = 45.0f;

GLenum glCheckError_(const char* file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
		case GL_INVALID_ENUM:
			error = "INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			error = "INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			error = "INVALID_OPERATION";
			break;
		case GL_OUT_OF_MEMORY:
			error = "OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error = "INVALID_FRAMEBUFFER_OPERATION";
			break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d, and height: %d\n", width, height);

	if (width > 0 && height > 0) {
		// Update the viewport size
		glViewport(0, 0, width, height);

		// Update the projection matrix with the new aspect ratio
		projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 1000.0f);
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
	}
}

int renderMode = 0;

void switchRenderMode(int mode) {
	switch (mode) {
	case 0:
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Solid mode
		break;
	case 1:
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
		break;
	case 2:
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); // Poligonal
		break;
	}
}

bool night = false;
bool rainEffect = false;


void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {

	if (automaticAnimationInProgress) {
		return;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		renderMode = (renderMode + 1) % 3;
		switchRenderMode(renderMode);
	}

	if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		// Increase light strength
		lightColor *= 1.6f;
		lightColor = glm::clamp(lightColor, 0.005f, 1.1f);

		if (lightColor.y > 0.05f && night) {
			pointLightColor = glm::vec3(0.0f, 0.0f, 0.0f);
			mySkyBox.Load(faces);
			night = false;
		}
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
		glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));
	}

	if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		// Decrease light strength
		lightColor *= 0.5f;
		lightColor = glm::clamp(lightColor, 0.005f, 1.1f);

		if (lightColor.y <= 0.05f && !night) {
			pointLightColor = glm::vec3(1.0f, 1.0f, 0.5f);
			mySkyBox.Load(faces2);
			night = true;
		}
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
		glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));
	}

	if (pressedKeys[GLFW_KEY_M]) {
		fogDensity += 0.003;
		if (fogDensity > 0.03)
			fogDensity = 0.03;
		myBasicShader.useShaderProgram();
		glUniform1fv(fogDensityLoc, 1, &fogDensity);
	}

	if (pressedKeys[GLFW_KEY_N]) {
		fogDensity -= 0.003;
		if (fogDensity <= 0)
			fogDensity = 0;
		myBasicShader.useShaderProgram();
		glUniform1fv(fogDensityLoc, 1, &fogDensity);
	}


	if (pressedKeys[GLFW_KEY_Z]) {
		lightAngle -= 1.0f;
	}

	if (pressedKeys[GLFW_KEY_X]) {
		lightAngle += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_R]) {
		rainEffect = 1 - rainEffect;
	}

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) {
			pressedKeys[key] = true;
		}
		else if (action == GLFW_RELEASE) {
			pressedKeys[key] = false;
		}
	}
}


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {

}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	if (automaticAnimationInProgress) {
		return;
	}

	//TODO
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	myCamera.rotate(yoffset, xoffset);

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCamera.rotate(pitch, yaw);
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
}
void processMovement() {

	view = myCamera.getViewMatrix();
	myBasicShader.useShaderProgram();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// Compute normal matrix for cartier
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}

	// Update model matrix for cartier
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
}

void initOpenGLWindow() {
	myWindow.Create(1924, 1055, "Speed through the Suburbs");

	glfwSetWindowPos(myWindow.getWindow(), 0, 35);

	// Set the resize callback function
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
	glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
	glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
	glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED); //Cursor off
}


void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise


}

void initModels() {
	cartier.LoadModel("models/cartier/cartier.obj");
	dodge.LoadModel("models/dodge/dodge.obj");
	eliceZ.LoadModel("models/eliceZ/eliceZ.obj");
	eliceY.LoadModel("models/eliceY/eliceY.obj");
	rain.LoadModel("models/rain/rain.obj");

}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
	myBasicShader.useShaderProgram();
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depthMapShader.vert", "shaders/depthMapShader.frag");
	depthMapShader.useShaderProgram();
}

void initPointLights() {
	// X = X // Y = Z // Z = - Y // OpenGL -> Blender
	pointLight[0] = glm::vec3(0.328f, 8.754f, -0.139f);
	pointLight[1] = glm::vec3(-2.979, 8.954, 21.839);
	pointLight[2] = glm::vec3(3.596, 8.970, 41.187);
	pointLight[3] = glm::vec3(26.455, 8.963, 3.246);
	pointLight[4] = glm::vec3(51.690, 8.967, -3.230);
	pointLight[5] = glm::vec3(-22.946, 9.074, -3.217);
	pointLight[6] = glm::vec3(-41.214, 8.971, 3.248);
	pointLight[7] = glm::vec3(-2.952, 8.970, -22.456);
	pointLight[8] = glm::vec3(3.542, 8.967, -47.387);
	pointLight[9] = glm::vec3(-30.462, 8.857, -42.352);
	pointLight[10] = glm::vec3(-41.335, 8.834, -42.095);
	pointLight[11] = glm::vec3(-52.796, 8.852, -41.965);
}

void initUniforms() {
	myBasicShader.useShaderProgram();

	// create model matrix for cartier
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
	// send view matrix to shader
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// compute normal matrix for cartier
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
		(float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
		0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	initPointLights();
	//set the light direction (direction towards the object)
	pointLightLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLight");
	// send light dir to shader
	glUniform3fv(pointLightLoc, 12, glm::value_ptr(pointLight[0]));

	//set light color
	pointLightColor = glm::vec3(0.0f, 0.0f, 0.0f); //first off then yellow
	pointLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightColor");
	// send light color to shader
	glUniform3fv(pointLightColorLoc, 1, glm::value_ptr(pointLightColor));

	// Fog
	fogDensity = 0.0f;
	fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
	glUniform1fv(fogDensityLoc, 1, &fogDensity);

}

void initSkybox() {
	faces.push_back("skybox/right.jpg");
	faces.push_back("skybox/left.jpg");
	faces.push_back("skybox/top.jpg");
	faces.push_back("skybox/bottom.jpg");
	faces.push_back("skybox/back.jpg");
	faces.push_back("skybox/front.jpg");

	faces2.push_back("skybox2/right.tga");
	faces2.push_back("skybox2/left.tga");
	faces2.push_back("skybox2/top.tga");
	faces2.push_back("skybox2/bottom.tga");
	faces2.push_back("skybox2/back.tga");
	faces2.push_back("skybox2/front.tga");
	mySkyBox.Load(faces);
}

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO

	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);
	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	//TODO - Return the light-space transformation matrix
	glm::mat4 lightView = glm::lookAt(glm::mat3(lightRotation) * lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = -100.0f, far_plane = 100.0f;
	glm::mat4 lightProjection = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;

	return lightSpaceTrMatrix;
}

float rainY = -50.0f;
float fallSpeed = 75.0f;
float lastFrameTime = 0.0f;
float groundLevel = -80.0f;

int gridRows = 20;
int gridCols = 20;

int numDropsPerGrid = 15;

void renderRain(gps::Shader shader) {
	float currentTime = glfwGetTime();
	float deltaTime = currentTime - lastFrameTime;

	rainY -= fallSpeed * deltaTime;

	if (rainY < groundLevel) {
		rainY = -50.0f;
	}

	float spacingX = 12.5f;
	float spacingZ = 12.5f;

	for (int i = 0; i < gridRows; i++) {
		for (int j = 0; j < gridCols; j++) {
			float currRainX = -190.0f + i * spacingX;
			float currRainZ = -190.0f + j * spacingZ;

			for (int k = 0; k < numDropsPerGrid; k++) {
				float offsetX = (k % 2 == 0) ? (k / 2) * 7.5f : -((k / 2) * 7.5f);
				float offsetZ = (k % 2 == 0) ? (k / 2) * 2.5f : -((k / 2) * 2.5f);
				float offsetY = k * 5.0f;

				glm::mat4 rainModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(currRainX + offsetX, rainY + offsetY, currRainZ + offsetZ));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(rainModelMatrix));
				rain.Draw(shader);
			}
		}
	}

	lastFrameTime = currentTime;
}

float dodgeRotation = 0.0f;
float eliceZRotation = 0.0f;
float eliceYRotation = 0.0f;

void renderAnimations(gps::Shader shader) {
	shader.useShaderProgram();

	// Set model matrix for Dodge
	glm::mat4 dodgeModelMatrix;
	dodgeModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, 0.0f, 0.0f));
	dodgeModelMatrix = glm::rotate(dodgeModelMatrix, glm::radians(dodgeRotation), glm::vec3(0, -1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(dodgeModelMatrix));

	// Draw Dodge
	dodge.Draw(shader);
	dodgeRotation -= 0.5;
	if (dodgeRotation <= -360)
		dodgeRotation = 0;

	// Set model matrix for eliceZ
	glm::mat4 eliceZModelMatrix;
	eliceZModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(10.2446f, 19.292f, -11.0723f));
	eliceZModelMatrix = glm::rotate(eliceZModelMatrix, glm::radians(eliceZRotation), glm::vec3(0, -1, 0));
	eliceZModelMatrix = glm::translate(eliceZModelMatrix, glm::vec3(-10.2446f, -19.292f, 11.0723f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(eliceZModelMatrix));

	// Draw eliceZ
	eliceZ.Draw(shader);
	eliceZRotation += 7.0;
	if (eliceZRotation >= 360)
		eliceZRotation = 0;

	// Set model matrix for eliceY
	glm::mat4 eliceYModelMatrix;
	eliceYModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(15.0883f, 18.0833f, -16.2909f));
	eliceYModelMatrix = glm::rotate(eliceYModelMatrix, glm::radians(51.0162f), glm::vec3(0, 1, 0));
	eliceYModelMatrix = glm::rotate(eliceYModelMatrix, glm::radians(eliceYRotation), glm::vec3(0, 0, 1));
	eliceYModelMatrix = glm::rotate(eliceYModelMatrix, glm::radians(-51.0162f), glm::vec3(0, 1, 0));
	eliceYModelMatrix = glm::translate(eliceYModelMatrix, glm::vec3(-15.0883f, -18.0833f, 16.2909f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(eliceYModelMatrix));

	// Draw eliceY
	eliceY.Draw(shader);
	eliceYRotation += 7.0;
	if (eliceYRotation >= 360)
		eliceYRotation = 0;
	
}

void drawObjects(gps::Shader shader, bool depthPass) {

	// select active shader program
	shader.useShaderProgram();

	//send model matrix data to shader
	modelLoc = glGetUniformLocation(shader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		normalMatrixLoc = glGetUniformLocation(shader.shaderProgram, "normalMatrix");
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
		lightDirLoc = glGetUniformLocation(shader.shaderProgram, "lightDir");
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));

	}

	cartier.Draw(shader);
	renderAnimations(shader);

}

void renderScene() {

	if (automaticAnimationInProgress) {
		float currentTime = glfwGetTime();
		float elapsedTime = currentTime - animationStartTime;

		if (elapsedTime < animationDuration) {
			glm::vec3 camPos = myCamera.getCameraPosition();
			camPos.x -= 0.2f;
			if (camPos.x < 0)
				camPos.y += 0.05f;
			myCamera.setCameraPosition(camPos);
		}
		else {
			automaticAnimationInProgress = false;
		}
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//render the scene to the depth buffer

	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	drawObjects(depthMapShader, true);


	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// final scene rendering pass (with shadows)

	glViewport(0, 0, 1920, 1080);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	myBasicShader.useShaderProgram();

	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//bind the shadow map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

	glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	mySkyBox.Draw(skyboxShader, view, projection);
	drawObjects(myBasicShader, false);
	if (rainEffect)
		renderRain(myBasicShader);

}

void cleanup() {
	myWindow.Delete();
	//cleanup code for your own data
	glDeleteTextures(1, &depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	//close GL context and any other GLFW resources
	glfwTerminate();
}


int main(int argc, const char* argv[]) {

	try {
		initOpenGLWindow();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// Set the mouse button and cursor position callbacks
	
	glfwSetMouseButtonCallback(myWindow.getWindow(), mouseButtonCallback);
	glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);

	initOpenGLState();
	switchRenderMode(renderMode);
	initModels();
	initShaders();
	initUniforms();
	setWindowCallbacks();
	initSkybox();
	initFBO();

	animationStartTime = glfwGetTime();
	

	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
		processMovement();
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		glCheckError();
	}


	cleanup();

	return EXIT_SUCCESS;
}
