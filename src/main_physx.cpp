#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "imagedata.h"
#include "glshader.h"
#include "glmeshdata.h"

#define PVD_HOST "127.0.0.1"	//Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.
#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL; }

#include "PxPhysicsAPI.h"
using namespace physx;

PxDefaultAllocator		gAllocator;
PxDefaultErrorCallback	gErrorCallback;

PxFoundation* gFoundation = NULL;
PxPhysics* gPhysics = NULL;

PxDefaultCpuDispatcher* gDispatcher = NULL;
PxScene* gScene = NULL;

PxMaterial* gMaterial = NULL;
PxPvd* gPvd = NULL;

struct physx_actor_entity
{
	PxRigidActor* actorPtr;
	PxU32 actorId;
};
std::vector<physx_actor_entity> physx_actors;

void initPhysics();
void stepPhysics();
void cleanupPhysics();

void createDynamic(const PxTransform & t, const PxGeometry & geometry, const PxVec3 & velocity = PxVec3(0))
{
	static PxU32 dynamicCounter = 0;

	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	gScene->addActor(*dynamic);

	physx_actors.push_back({ dynamic, dynamicCounter++ });
}

void createStack(const PxTransform & t, PxU32 size, PxReal halfExtent)
{
	PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *gMaterial);

	static PxU32 stackCounter = 0;
	for (PxU32 i = 0; i < size; i++)
	{
		for (PxU32 j = 0; j < size - i; j++)
		{
			PxTransform localTm(PxVec3(PxReal(j * 2) - PxReal(size - i), PxReal(i * 2 + 1), 0) * halfExtent);
			PxRigidDynamic* body = gPhysics->createRigidDynamic(t.transform(localTm));
			body->attachShape(*shape);
			PxRigidBodyExt::updateMassAndInertia(*body, 20.0f);
			gScene->addActor(*body);

			physx_actors.push_back({ body, stackCounter++ });
		}
	}

	shape->release();
}

void initPhysics()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
	gScene->addActor(*groundPlane);

	PxReal stackZ = 10.0f;
	for (PxU32 i = 0; i < 4; i++)
		createStack(PxTransform(PxVec3(0, 0, stackZ -= 10.0f)), 10, 2.0f);
}

void stepPhysics()
{
	gScene->simulate(1.0f / 60.0f);
	gScene->fetchResults(true);
}

void cleanupPhysics()
{
	PX_RELEASE(gScene);
	PX_RELEASE(gDispatcher);
	PX_RELEASE(gPhysics);
	if (gPvd)
	{
		PxPvdTransport* transport = gPvd->getTransport();
		gPvd->release();	gPvd = NULL;
		PX_RELEASE(transport);
	}
	PX_RELEASE(gFoundation);
}

GLFWwindow* window;
std::string g_app_title = "minimal_glfw_physx";

int g_width = 1024;
int g_height = 768;

// ogl attributes and variables
GLuint texture_crate = -1;
GLuint texture_checker = -1;

const int num_ball_textures = 15;
GLuint texIds[num_ball_textures]; // 15 ball

// camera transformation attributes
glm::mat4 g_view_matrix;
glm::mat4 g_proj_matrix;

glm::vec3 g_cam_position = glm::vec3(50, 20, 50);

float g_cam_horizontal_angle = -(3.0f/4.0f) * float(M_PI);
float g_cam_vertical_angle = 0.0f;
float g_cam_fov = (1.0f/4.0f) * float(M_PI);

// camera movement/turning attributes
float g_cam_move_speed = 10.0f;
float g_cam_turn_speed = 0.00075f;

float g_last_cursorpos_x = 0.0f;
float g_last_cursorpos_y = 0.0f;

std::string rootData = "";

static bool findFullPath(const std::string& root, std::string& filePath)
{
	bool fileFound = false;
	const std::string resourcePath = root;

	filePath = resourcePath + filePath;
	for (unsigned int i = 0; i < 16; ++i)
	{
		std::ifstream file;
		file.open(filePath.c_str());
		if (file.is_open())
		{
			fileFound = true;
			break;
		}

		filePath = "../" + filePath;
	}

	return fileFound;
}

void computeMatricesFromInputs()
{

	// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	float dx = float(x) - g_last_cursorpos_x;
	float dy = float(y) - g_last_cursorpos_y;

	g_last_cursorpos_x = float(x);
	g_last_cursorpos_y = float(y);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		// Compute new orientation
		g_cam_horizontal_angle -= g_cam_turn_speed * dx;
		g_cam_vertical_angle -= g_cam_turn_speed * dy;
	}

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
		std::cos(g_cam_vertical_angle) * std::sin(g_cam_horizontal_angle),
		std::sin(g_cam_vertical_angle),
		std::cos(g_cam_vertical_angle) * std::cos(g_cam_horizontal_angle)
	);

	glm::vec3 right = glm::vec3(
		std::sin(g_cam_horizontal_angle - 3.14f / 2.0f),
		0,
		std::cos(g_cam_horizontal_angle - 3.14f / 2.0f)
	);

	glm::vec3 up = glm::cross(right, direction);

	float l_speed = g_cam_move_speed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		l_speed *= 10.0f;
	}

	// Move forward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		g_cam_position += direction * deltaTime * l_speed;
	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		g_cam_position -= direction * deltaTime * l_speed;
	}
	// Strafe right
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		g_cam_position += right * deltaTime * l_speed;
	}
	// Strafe left
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		g_cam_position -= right * deltaTime * l_speed;
	}

	// Y DOWN
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		g_cam_position -= up * deltaTime * l_speed;
	}
	// Y UP
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		g_cam_position += up * deltaTime * l_speed;
	}

	g_proj_matrix = glm::perspective(g_cam_fov, float(g_width) / float(g_height), 0.25f, 4000.0f);
	g_view_matrix = glm::lookAt(g_cam_position, g_cam_position + direction, up);

	lastTime = currentTime;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		glm::vec3 direction(
			std::cos(g_cam_vertical_angle) * std::sin(g_cam_horizontal_angle),
			std::sin(g_cam_vertical_angle),
			std::cos(g_cam_vertical_angle) * std::cos(g_cam_horizontal_angle)
		);

		createDynamic(PxTransform(PxVec3(g_cam_position.x, g_cam_position.y, g_cam_position.z)), PxSphereGeometry(4), PxVec3(direction.x, direction.y, direction.z)*175.0f);
	}
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	g_width = width;
	g_height = height;
}

int main(void)
{
	{
		std::string locStr = "resources.loc";
		size_t len = locStr.size();

		bool fileFound = findFullPath("./resources/", locStr);
		rootData = locStr.substr(0, locStr.size() - len);
	}

	initPhysics();

	// initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "glfwInit failed.\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// open a window and create its OpenGL context
	window = glfwCreateWindow(g_width, g_height, g_app_title.c_str(), NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "glfwCreateWindow failed.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// set glfw callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// initialize GLEW
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "glewInit failed.\n");
		glfwTerminate();
		return -1;
	}

	// Output some info on the OpenGL implementation
	const GLubyte* glvendor = glGetString(GL_VENDOR);
	const GLubyte* glrenderer = glGetString(GL_RENDERER);
	const GLubyte* glversion = glGetString(GL_VERSION);

	printf("GL_VENDOR: %s\n", glvendor);
	printf("GL_RENDERER: %s\n", glrenderer);
	printf("GL_VERSION: %s\n", glversion);

	// set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, g_width / 2, g_height / 2);

	// set ogl states and defaults
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);

	GLMeshData myPlane;
	myPlane.createPlane(0.0f, 128.0f, 2.0f);

	GLMeshData myBox;
	myBox.createBox(4.0f, 4.0f, 4.0f);
	
	GLMeshData mySphere;
	mySphere.createSphere(4.0f, 32, 32);

	{
		ImageData image;
		image.loadBMP(rootData + "textures/crate.bmp");

		glGenTextures(1, &texture_crate);
		glBindTexture(GL_TEXTURE_2D, texture_crate);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.getWidth(), image.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.getData());
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	{
		ImageData image;
		image.loadBMP(rootData + "textures/checker.bmp");

		glGenTextures(1, &texture_checker);
		glBindTexture(GL_TEXTURE_2D, texture_checker);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.getWidth(), image.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.getData());
		glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	{
		for (int i = 0; i < num_ball_textures; ++i)
		{
			std::stringstream ss;
			ss << std::setfill('0') << std::setw(2) << std::fixed << i+1;

			std::stringstream baseNameSS;
			baseNameSS << rootData + "textures/pool/";
			baseNameSS << "pool_";
			baseNameSS << ss.str();
			baseNameSS << ".ppm";

			ImageData image;
			image.loadPBM(baseNameSS.str());

			glGenTextures(1, texIds + i);
			glBindTexture(GL_TEXTURE_2D, texIds[i]);

			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, image.getWidth(), image.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.getData());
			glGenerateMipmap(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders((rootData + "shaders/TransformVertexShader.vertexshader").c_str(), (rootData + "shaders/TextureFragmentShader.fragmentshader").c_str());

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

	double lastFPStime = glfwGetTime();
	int frameCounter = 0;

	do {
		double thisFPStime = glfwGetTime();
		frameCounter++;

		if (thisFPStime - lastFPStime >= 1.0)
		{
			lastFPStime = thisFPStime;

			std::string windowTitle = g_app_title + " (";
			windowTitle += std::to_string(frameCounter);
			windowTitle += " fps)";
			const char* windowCaption = windowTitle.c_str();
			glfwSetWindowTitle(window, windowCaption);

			frameCounter = 0;
		}

		stepPhysics();

		glViewport(0, 0, g_width, g_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// bind shader
		glUseProgram(programID);

		// compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();

		// render physx shapes
		{
			const int MAX_NUM_ACTOR_SHAPES = 128;
			
			PxShape* shapes[MAX_NUM_ACTOR_SHAPES];
			for (PxU32 i = 0; i < static_cast<PxU32>(physx_actors.size()); i++)
			{
				const PxU32 nbShapes = physx_actors[i].actorPtr->getNbShapes();
				PX_ASSERT(nbShapes <= MAX_NUM_ACTOR_SHAPES);
				physx_actors[i].actorPtr->getShapes(shapes, nbShapes);
			
				for (PxU32 j = 0; j < nbShapes; j++)
				{
					const PxMat44 shapePose(PxShapeExt::getGlobalPose(*shapes[j], *physx_actors[i].actorPtr));
					const PxGeometryHolder h = shapes[j]->getGeometry();
			
					// render object
					glm::mat4 model_matrix = glm::make_mat4(&shapePose.column0.x);
					glm::mat4 mvp_mat = g_proj_matrix * g_view_matrix * model_matrix;
			
					if (h.getType() == PxGeometryType::eBOX)
					{
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, texture_crate);
						glUniform1i(TextureID, 0);
			
						glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(mvp_mat));
						myBox.render();
					}
					else if (h.getType() == PxGeometryType::eSPHERE)
					{
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, texIds[physx_actors[i].actorId % 15]);
						glUniform1i(TextureID, 0);
			
						glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(mvp_mat));
						mySphere.render();
					}
				}
			}
		}

		// render ground plane
		{
			glm::mat4 model_matrix = glm::mat4(1.0);
			glm::mat4 mvp_mat = g_proj_matrix * g_view_matrix * model_matrix;

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture_checker);
			glUniform1i(TextureID, 0);
			
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(mvp_mat));
			myPlane.render();
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	}
	while (glfwWindowShouldClose(window) == 0);

	// cleanup mesh, shader and texture ogl resources
	myPlane.clear();
	myBox.clear();
	mySphere.clear();

	glDeleteProgram(programID);

	glDeleteTextures(1, &texture_crate);
	glDeleteTextures(1, &texture_checker);
	glDeleteTextures(num_ball_textures, texIds);

	// close ogl window and terminate glfw
	glfwDestroyWindow(window);
	// finalize and clean up glfw
	glfwTerminate();

	cleanupPhysics();

	return 0;
}