// MD2 animation renderer
// This demo will load and render an animated MD2 model, an OBJ model and a skybox
// Most of the OpenGL code for dealing with buffer objects, etc has been moved to a 
// utility library, to make creation and display of mesh objects as simple as possible

// Windows specific: Uncomment the following line to open a console window for debug output
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

// Sources referenced stated below
// Source: https://learnopengl.com/Lighting/Multiple-lights
// Source: https://www.tomdalling.com/blog/modern-opengl/
// Source: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// Source: Matteo Marcuzzo - https://www.youtube.com/watch?v=4aPGz9JypQg&t=85s - Shadow Mapping Demonstration

// Globals
// Real programs don't use globals :-D

#include "rt3d.h"
#include "rt3dObjLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#include "md2model.h"

using namespace std;

#define DEG_TO_RADIAN 0.017453293

GLuint meshIndexCount = 0;
GLuint md2VertCount = 0;
GLuint meshObjects[3];
GLuint textureProgram;

// Globals for shadow and shader programs

GLuint shadowMappingProgram;
GLuint shaderProgram;

GLfloat r = 0.0f;

// used for camera, taken from AGP Module

glm::vec3 eye(-2.0f, 1.0f, 8.0f);
glm::vec3 at(0.0f, 1.0f, -1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);

stack<glm::mat4> mvStack;


// Frame buffer globals for shadows 
// the code below was influenced by https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping

GLuint depthMapFBO; // Frame buffer
GLuint depthMap;	// Frame buffer texture
const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
const GLuint screenWidth = 800, screenHeight = 600;

// TEXTURE STUFF
GLuint textures[4];

// this was taken from Lab 4 base code provided wihtin the agb module
// this is used here to structure the main light source for creating shadows within the shadow demo

rt3d::lightStruct light0 = {
	{ 0.4f, 0.4f, 0.4f, 1.0f }, // ambient
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // diffuse
	{ 1.0f, 1.0f, 1.0f, 1.0f }, // specular
	{ -5.0f, 2.0f, 2.0f, 1.0f }  // position
};

glm::vec4 lightPos(-6.0f, 5.0f, 4.0f, 1.0f);
glm::vec3 lightPos3(0.0f, 0.0f, 0.0f);

GLuint uniformIndex; // used for multiple lights

// The following code is sourced from my previous project, which demonstrated multiple lights | B00298673's Individual Project
// Global declaration for switching between the 3 light sources (RED | GREEN | BLUE)

// RED cube

rt3d::lightStruct RedLight = {

	{ 1.0f, 0.0f, 0.0f, 1.0f }, // ambient
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // diffuse
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // specular
	{ -5.0f, 2.0f, 2.0f, 1.0f },  // position
	1.0f, 0.09f, 0.032f

// an extra three parameters have been added - Light attenuation - In order of Constant, Linear and Quadratic

}; glm::vec4 RedLightPos(6.5f, 7.5f, 8.8f, 1.0f); // Red Light's pos

// GREEN cube

rt3d::lightStruct GreenLight = {

	{ 0.0f, 1.0f, 0.0f, 1.0f }, // ambient
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // diffuse
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // specular
	{ 5.0f, 2.0f, 2.0f, 1.0f },  // position
	1.0f, 0.09f, 0.032f

// an extra three parameters have been added - Light attenuation - In order of Constant, Linear and Quadratic

}; glm::vec4 GreenLightPos(4.0f, 3.5f, 8.8f, 1.0f); // Green Light's position

// BLUE cube

rt3d::lightStruct BlueLight = {

	{ 0.0f, 0.0f, 1.0f, 1.0f }, // ambient
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // diffuse
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // specular
	{ -5.0f, 2.0f, 12.0f, 1.0f },  // position
	1.0f, 0.09f, 0.032f

// an extra three parameters have been added - Light attenuation - In order of Constant, Linear and Quadratic

}; glm::vec4 BlueLightPos(9.0f, 3.5f, 8.8f, 1.0f); // Blue Light's position

// material struct is used for the multiple light cubes within the scene, it can also be used for other objects within the draw function

rt3d::materialStruct material0 = {

	{ 1.0f, 1.0f, 1.0f, 1.0f }, // ambient
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // diffuse
	{ 0.0f, 0.0f, 0.0f, 1.0f }, // specular
	1.0f  // shininess
};

float theta = 0.0f;

// md2 stuff
md2model tmpModel;

// Set up rendering context

SDL_Window * setupRC(SDL_GLContext &context) {
	SDL_Window * window;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) // Initialize video
		rt3d::exitFatalError("Unable to initialize SDL");

	// Request an OpenGL 3.0 context.

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // double buffering on
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // 8 bit alpha buffering
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // Turn on x4 multisampling anti-aliasing (MSAA)

													   // Create 800x600 window
	window = SDL_CreateWindow("SDL/GLM/OpenGL Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!window) // Check window was created OK
		rt3d::exitFatalError("Unable to create window");

	context = SDL_GL_CreateContext(window); // Create opengl context and attach to window
	SDL_GL_SetSwapInterval(1); // set swap buffers to sync with monitor's vertical refresh rate
	return window;
}

// A simple texture loading function
// lots of room for improvement - and better error checking!
GLuint loadBitmap(char *fname) {
	GLuint texID;
	glGenTextures(1, &texID); // generate texture ID

							  // load file - using core SDL library
	SDL_Surface *tmpSurface;
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface) {
		std::cout << "Error loading bitmap" << std::endl;
	}

	// bind texture and set parameters
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_PixelFormat *format = tmpSurface->format;

	GLuint externalFormat, internalFormat;
	if (format->Amask) {
		internalFormat = GL_RGBA;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGRA;
	}
	else {
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tmpSurface->w, tmpSurface->h, 0,
		externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(tmpSurface); // texture loaded, free the temporary buffer
	return texID;	// return value of texture ID
}

// Below is the function used for initialising shaders, textures, light, shadow... programs

void init(void) {

	// shadow Program
	shadowMappingProgram = rt3d::initShaders("shadowMap.vert", "shadowMap.frag");

	// multiple lighting program
	shaderProgram = rt3d::initShaders("MultipleLighting.vert", "MultipleLighting.frag");
	rt3d::setLight(shaderProgram, RedLight, 0);
	rt3d::setLight(shaderProgram, GreenLight, 1);
	rt3d::setLight(shaderProgram, BlueLight, 2);
	rt3d::setMaterial(shaderProgram, material0);

	textureProgram = rt3d::initShaders("textured.vert", "textured.frag");

	vector<GLfloat> verts;
	vector<GLfloat> norms;
	vector<GLfloat> tex_coords;

	vector<GLuint> indices;
	rt3d::loadObj("cube.obj", verts, norms, tex_coords, indices);
	meshIndexCount = indices.size();
	textures[0] = loadBitmap("Spot-Tex.bmp");
	meshObjects[0] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), meshIndexCount, indices.data());

	textures[1] = loadBitmap("hobgoblin2.bmp");
	meshObjects[1] = tmpModel.ReadMD2Model("tris.MD2");
	md2VertCount = tmpModel.getVertDataCount();

	textures[2] = loadBitmap("Smooth.bmp");
	textures[3] = loadBitmap("BaseTex.bmp");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// Frame buffer for shadows 
// the code below was influence by Matteo Marcuzzo's individual project 
// and https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping

	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);

	glBindTexture(GL_TEXTURE_2D, depthMap);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);

	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

}

// the code below is camera movement, taken from AGP Module

glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::sin(r*DEG_TO_RADIAN), pos.y, pos.z - d*std::cos(r*DEG_TO_RADIAN));
}

glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::cos(r*DEG_TO_RADIAN), pos.y, pos.z + d*std::sin(r*DEG_TO_RADIAN));
}

// below is the code used for controlling the camera and light source
// movement for light source was neccessary for demonstrating shadows based
// on the light's position

void update(void) {

	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_W]) eye = moveForward(eye, r, 0.1f);
	if (keys[SDL_SCANCODE_S]) eye = moveForward(eye, r, -0.1f);
	if (keys[SDL_SCANCODE_A]) eye = moveRight(eye, r, -0.1f);
	if (keys[SDL_SCANCODE_D]) eye = moveRight(eye, r, 0.1f);
	if (keys[SDL_SCANCODE_R]) eye.y += 0.1;
	if (keys[SDL_SCANCODE_F]) eye.y -= 0.1;

	if (keys[SDL_SCANCODE_UP]) lightPos[2] -= 0.1;
	if (keys[SDL_SCANCODE_LEFT]) lightPos[0] -= 0.1;
	if (keys[SDL_SCANCODE_DOWN]) lightPos[2] += 0.1;
	if (keys[SDL_SCANCODE_RIGHT]) lightPos[0] += 0.1;
	if (keys[SDL_SCANCODE_O]) lightPos[1] += 0.1;
	if (keys[SDL_SCANCODE_P]) lightPos[1] -= 0.1;

	if (keys[SDL_SCANCODE_COMMA]) r -= 1.0f;
	if (keys[SDL_SCANCODE_PERIOD]) r += 1.0f;

	for (int i = 0; i < 3; i++)
		lightPos3[i] = lightPos[i];

	if (keys[SDL_SCANCODE_1]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
	}
	if (keys[SDL_SCANCODE_2]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}

}

// the code below has been influenced by Matteo Marcuzzo's Individual project
// each object has it's own function as it will make rendering shadows easier

void groundPlane(GLuint shader) 
{
	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-10.0f, -5.5f, -10.0f));
	model = glm::scale(model, glm::vec3(22.5f, 5.5f, 22.5f));
	rt3d::setUniformMatrix4fv(shader, "model", glm::value_ptr(model));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
}

void renderRGBWall(GLuint shader)
{

	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(7.0f, 3.0f, 1.2f));
	model = glm::scale(model, glm::vec3(3.0f, 3.0f, 0.5f));
	rt3d::setUniformMatrix4fv(shader, "model", glm::value_ptr(model));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);

}


void pillars(GLuint shader)
{

	glm::mat4 model;
	for (int b = 0; b <7; b++) {
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(-17.5f + b * 2, 1.0f, -12.0f + b * 2));
		model = glm::scale(model, glm::vec3(0.6f, 1.0f + b, 0.6f));
		rt3d::setUniformMatrix4fv(shader, "model", glm::value_ptr(model));
		rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);

	}
}

void goblinOne(GLuint shader) 

{

	// draw the hobgoblin
	glCullFace(GL_FRONT); // md2 faces are defined clockwise, so cull front face

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-15.5f, 1.15f, -3.5f));
	model = glm::rotate(model, float(90.0f*DEG_TO_RADIAN), glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0*0.05, 1.0*0.05, 1.0*0.05));
	rt3d::setUniformMatrix4fv(shader, "model", glm::value_ptr(model));
	rt3d::drawMesh(meshObjects[1], md2VertCount, GL_TRIANGLES);


	glCullFace(GL_BACK);

}

void goblinTwo(GLuint shader)

{

	// draw the hobgoblin
	glCullFace(GL_FRONT); // md2 faces are defined clockwise, so cull front face

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-10.0f, 1.15f, 1.0f));
	model = glm::rotate(model, float(90.0f*DEG_TO_RADIAN), glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0*0.05, 1.0*0.05, 1.0*0.05));
	rt3d::setUniformMatrix4fv(shader, "model", glm::value_ptr(model));
	rt3d::drawMesh(meshObjects[1], md2VertCount, GL_TRIANGLES);


	glCullFace(GL_BACK);

}

	// the code below has been influenced by Matteo Marcuzzo's Individual project
	// function below is made for rendering objects within the scene which will have shadows
	// multiple lighting objects are not included within this scene as it wasn't neccessary
	// shadows will be created for the pillar and goblin objects within the scene

void shadowDemo(glm::mat4 projection, glm::mat4 lightSpaceMatrix, glm::mat4 viewMatrix, GLuint shader) {


	glUseProgram(shader);
	rt3d::setUniformMatrix4fv(shader, "lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
	rt3d::setUniformMatrix4fv(shader, "projection", glm::value_ptr(projection));
	rt3d::setUniformMatrix4fv(shader, "view", glm::value_ptr(viewMatrix));

	GLuint uniformIndex = glGetUniformLocation(shader, "lightPos");
	glUniform3fv(uniformIndex, 1, glm::value_ptr(lightPos3));
	uniformIndex = glGetUniformLocation(shader, "viewPos");
	glUniform3fv(uniformIndex, 1, glm::value_ptr(eye));
	uniformIndex = glGetUniformLocation(shader, "diffuseTexture");
	glUniform1i(uniformIndex, 1);
	uniformIndex = glGetUniformLocation(shader, "shadowMap");
	glUniform1i(uniformIndex, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[3]);

	// Below is the only flaw within this whole project
	// drawing the shadow demo objects before the glActiveTexture call, will cause the shadows to stop working although the RGB lights working
	// drawing these objects after will cause the RGB lights to stop working and shadows to start working

	// keep render object code bellow UNCOMMENTED if you want the multiple lights to show their true colours, Green, Red and Blue (RGB Cubes)
	// also - render objects code below should then be COMMENTED to demonstrate multiple lights (RGB Cubes)

	//groundPlane(shader);
	//pillars(shader);
	//goblinOne(shader);
	//goblinTwo(shader);


	glActiveTexture(GL_TEXTURE0);

	// keep render object code below UNCOMMENTED if you want to demonstrate the shadow map shader 
	// also - render objects code above should then be COMMENTED to demonstrate Shadows

	groundPlane(shader);
	pillars(shader);
	goblinOne(shader);
	goblinTwo(shader);


	glBindTexture(GL_TEXTURE_2D, depthMap);
	glUseProgram(shaderProgram);
	rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));
	// Code sourced from B00298673's Individual Project
	
	// RedLight
	// This is used to draw each individual light souce in the scene
	// Similar code should be used for each, just altering the colour its assigned to

	glm::vec4 tmp = mvStack.top()*RedLightPos;
	RedLight.position[0] = tmp.x;
	RedLight.position[1] = tmp.y;
	RedLight.position[2] = tmp.z;
	rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp), 0);

	// GreenLight
	glm::vec4 tmp1 = mvStack.top()*GreenLightPos;
	GreenLight.position[0] = tmp1.x;
	GreenLight.position[1] = tmp1.y;
	GreenLight.position[2] = tmp1.z;
	rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp1), 1);

	// BlueLight
	glm::vec4 tmp2 = mvStack.top()*BlueLightPos;
	BlueLight.position[0] = tmp2.x;
	BlueLight.position[1] = tmp2.y;
	BlueLight.position[2] = tmp2.z;
	rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp2), 2);

	
	// draw a small cube block at lightPos - repeating this code 3 times, as there are 3 lights

	// code below was taken from lab 4 base code, it has been adapted slightly for this project
	// this code was taken from my individual project

	// RedLight
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(RedLightPos[0], RedLightPos[1], RedLightPos[2]));
	mvStack.top() = glm::scale(mvStack.top(),glm::vec3(0.75f, 0.75f, 0.75f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0],meshIndexCount,GL_TRIANGLES);
	mvStack.pop();
	
	// GreenLight
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(GreenLightPos[0], GreenLightPos[1], GreenLightPos[2]));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(0.75f, 0.75f, 0.75f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// BlueLight
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(BlueLightPos[0], BlueLightPos[1], BlueLightPos[2]));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(0.75f, 0.75f, 0.75f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// RGB Wall to demonstrate RGB Colour 'wheel' effect

	glBindTexture(GL_TEXTURE_2D, textures[2]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(6.5f, 6.0f, 7.5f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(6.0f, 6.0f, 0.5f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();


}

void draw(SDL_Window * window) {

	// clear the screen
	glEnable(GL_CULL_FACE);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection(1.0);
	projection = glm::perspective(float(60.0f*DEG_TO_RADIAN), 800.0f / 600.0f, 1.0f, 150.0f);
	
	GLfloat scale(1.0f); // just to allow easy scaling of complete scene

	glm::mat4 modelview(1.0); // set base position for scene
	mvStack.push(modelview);

	// this code is used for the camera's position within the scene
	at = moveForward(eye, r, 1.0f);
	mvStack.top() = glm::lookAt(eye, at, up);

	// the code below was influence by Matteo Marcuzzo's individual project 
	// and https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
	// these calls are needed for the shadowMapping program

	glm::mat4 lightProjection, lightView;
	GLfloat near_plane = -10.0f, far_plane = 20.0f;

	lightProjection = glm::ortho<float>(-20, 0, -10, 10, near_plane, far_plane);
	lightView = glm::lookAt(lightPos3, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 lightSpaceMatrix;
	lightSpaceMatrix = lightProjection * lightView;

	// the loop below renders the shadow demonstration within two passes

	for (int pass = 0; pass < 2; pass++)
	
	{

		if (pass == 0)

		{

			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear FBO
			glClear(GL_DEPTH_BUFFER_BIT);

			glCullFace(GL_FRONT); // cull front faces when drawing to shadow map, to fix some issues
			shadowDemo(lightProjection, lightSpaceMatrix, lightView, shadowMappingProgram); // render using light's point of view
			glCullFace(GL_BACK); // don't forget to reset original culling face

		}

		else 
		
		{
			//Render to frame buffer
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glViewport(0, 0, screenWidth, screenHeight);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear window
																// clear the screen
			glEnable(GL_CULL_FACE);
			glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
			shadowDemo(projection, lightSpaceMatrix, mvStack.top(), shadowMappingProgram); // render normal scene from normal point of view

		}

		glDepthMask(GL_TRUE);
	}

	SDL_GL_SwapWindow(window); // swap buffers
}

// Program entry point - SDL manages the actual WinMain entry point for us
int main(int argc, char *argv[]) {
	SDL_Window * hWindow; // window handle
	SDL_GLContext glContext; // OpenGL context handle
	hWindow = setupRC(glContext); // Create window and render context 

								  // Required on Windows *only* init GLEW to access OpenGL beyond 1.1
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) { // glewInit failed, something is seriously wrong
		std::cout << "glewInit failed, aborting." << endl;
		exit(1);
	}
	cout << glGetString(GL_VERSION) << endl;

	init();

	bool running = true; // set running to true
	SDL_Event sdlEvent;  // variable to detect SDL events
	while (running) {	// the event loop
		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_QUIT)
				running = false;
		}
		update();
		draw(hWindow); // call the draw function
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(hWindow);
	SDL_Quit();
	return 0;
}