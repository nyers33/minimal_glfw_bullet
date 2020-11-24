#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "imagedata.h"
#include "glshader.h"
#include "glmeshdata.h"

// bt
#include "btBulletDynamicsCommon.h"
#include <stdio.h>

btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btBroadphaseInterface* overlappingPairCache;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld;

// collision shape array, release memory at exit
btAlignedObjectArray<btCollisionShape*> collisionShapes;

void initPhysics();
void stepPhysics();
void cleanupPhysics();

void initPhysics()
{
	// collision configuration contains default setup for memory, collision setup
	collisionConfiguration = new btDefaultCollisionConfiguration();

	// default collision dispatcher
	dispatcher = new btCollisionDispatcher(collisionConfiguration);

	// general purpose broadphase
	overlappingPairCache = new btDbvtBroadphase();

	// default constraint solver
	solver = new btSequentialImpulseConstraintSolver;

	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

	dynamicsWorld->setGravity(btVector3(0, -10, 0));

	// create a few basic rigid bodies

	// ground plane
	{
		btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(btScalar(0), btScalar(1), btScalar(0)), btScalar(0));

		collisionShapes.push_back(groundShape);

		btTransform groundTransform;
		groundTransform.setIdentity();
		groundTransform.setOrigin(btVector3(0, 0, 0));

		btScalar mass(0.f);

		// rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			groundShape->calculateLocalInertia(mass, localInertia);

		// using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		// add the body to the dynamics world
		dynamicsWorld->addRigidBody(body);
	}
	
	{
		btCollisionShape* colShape;
		btTransform startTransform;
		btScalar mass(0.25f);
		float rad = 12.0f;

		for (int j = 0; j < 24; j++)
		{
			for (int i = 0; i < 16; i++)
			{
				colShape = new btBoxShape(btVector3(btScalar(1.125), btScalar(1.0), btScalar(2.0)));
				collisionShapes.push_back(colShape);

				startTransform.setIdentity();

				// rigidbody is dynamic if and only if mass is non zero, otherwise static
				bool isDynamic = (mass != 0.f);

				btVector3 localInertia(0, 0, 0);
				if (isDynamic)
					colShape->calculateLocalInertia(mass, localInertia);

				startTransform.setOrigin(btVector3(rad * cos(2.0f * static_cast<float>(M_PI) * (i + static_cast<float>(j % 2) / 2.0f) / 16.0f), 1.0f + j * 2.0f, -rad * sin(2.0f * static_cast<float>(M_PI) * (i + static_cast<float>(j % 2) / 2.0f) / 16.0f)));
				startTransform.setRotation(btQuaternion(btVector3(btScalar(0), btScalar(1), btScalar(0)), btScalar((i + static_cast<float>(j % 2) / 2.0f) * 2.0f * static_cast<float>(M_PI) / 16.0f)));

				// using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
				btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
				btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
				btRigidBody* body = new btRigidBody(rbInfo);

				dynamicsWorld->addRigidBody(body);
			}
		}
	}
}

void cleanupPhysics()
{
	// cleanup in the reverse order of creation/initialization

	// remove the rigidbodies from the dynamics world and delete them
	for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
	{
		btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}
		dynamicsWorld->removeCollisionObject(obj);
		delete obj;
	}

	// delete collision shapes
	for (int j = 0; j < collisionShapes.size(); j++)
	{
		btCollisionShape* shape = collisionShapes[j];
		collisionShapes[j] = 0;
		delete shape;
	}

	// delete dynamics world
	delete dynamicsWorld;

	// delete solver
	delete solver;

	// delete broadphase
	delete overlappingPairCache;

	// delete dispatcher
	delete dispatcher;

	delete collisionConfiguration;

	// next line is optional: it will be cleared by the destructor when the array goes out of scope
	collisionShapes.clear();
}

void stepPhysics()
{
	dynamicsWorld->stepSimulation(1.f / 60.f, 10);
}

GLFWwindow* window;
std::string g_app_title = "minimal_glfw_bullet";

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

glm::vec3 g_cam_position = glm::vec3(50, 30, 50);

float g_cam_horizontal_angle = -(3.0f / 4.0f) * float(M_PI);
float g_cam_vertical_angle = 0.0f;
float g_cam_fov = (1.0f / 3.0f) * float(M_PI);

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

void fireSphere(btVector3 pos, btVector3 dir, float speed)
{
	// sphere
	{
		// create a dynamic rigidbody
		btCollisionShape* colShape = new btSphereShape(btScalar(1.));
		collisionShapes.push_back(colShape);

		// create dynamic objects
		btTransform startTransform;
		startTransform.setIdentity();

		btScalar mass(1.f);

		// rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);

		startTransform.setOrigin(pos);

		// using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
		btRigidBody* body = new btRigidBody(rbInfo);

		body->setLinearVelocity(dir * speed);

		dynamicsWorld->addRigidBody(body);
	}
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

		glm::vec3 right = glm::vec3(
			std::sin(g_cam_horizontal_angle - 3.14f / 2.0f),
			0,
			std::cos(g_cam_horizontal_angle - 3.14f / 2.0f)
		);

		glm::vec3 up = glm::cross(right, direction);

		fireSphere(btVector3(g_cam_position.x - 7.5f * right.x, g_cam_position.y - 10.0f, g_cam_position.z - 7.5f * right.z), btVector3(direction.x, direction.y, direction.z), 100.0f);
		fireSphere(btVector3(g_cam_position.x				   , g_cam_position.y - 7.5f , g_cam_position.z), btVector3(direction.x, direction.y, direction.z), 100.0f);
		fireSphere(btVector3(g_cam_position.x + 7.5f * right.x, g_cam_position.y - 10.0f, g_cam_position.z + 7.5f * right.z), btVector3(direction.x, direction.y, direction.z), 100.0f);
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

	// output some info on the OpenGL implementation
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
	myBox.createBox(2.25f, 2.0f, 4.0f);

	GLMeshData mySphere;
	mySphere.createSphere(1.0f, 32, 32);

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
			ss << std::setfill('0') << std::setw(2) << std::fixed << i + 1;

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

	// create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders((rootData + "shaders/TransformVertexShader.vertexshader").c_str(), (rootData + "shaders/TextureFragmentShader.fragmentshader").c_str());

	// get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	// get a handle for our "myTextureSampler" uniform
	GLuint ColorID = glGetUniformLocation(programID, "albedo");

	glm::vec3 albedo  = glm::vec3(0.5f, 0.5f, 0.5f);
	glm::vec3 albedoR = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 albedoG = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 albedoB = glm::vec3(0.0f, 0.0f, 1.0f);

	glm::vec3 albedoArray[] = { albedoR, albedoG, albedoB };

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

		// render collsion shapes
		{
			for (int j = dynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--)
			{
				btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[j];
				btRigidBody* body = btRigidBody::upcast(obj);
				btTransform transform;
				if (body && body->getMotionState())
				{
					body->getMotionState()->getWorldTransform(transform);
				}
				else
				{
					transform = obj->getWorldTransform();
				}

				// convert the btTransform into the GLM matrix using 'glm::value_ptr'
				glm::mat4 model_matrix;
				transform.getOpenGLMatrix(glm::value_ptr(model_matrix));

				// model-view-projection
				glm::mat4 mvp_mat = g_proj_matrix * g_view_matrix * model_matrix;

				if (strcmp((obj->getCollisionShape())->getName(), "Box") == 0)
				{
					glUniform3fv(ColorID, 1, glm::value_ptr(albedoArray[j % 3]));

					glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(mvp_mat));
					myBox.render();
				}
				else if (strcmp((obj->getCollisionShape())->getName(), "SPHERE") == 0)
				{
					glUniform3fv(ColorID, 1, glm::value_ptr(albedoArray[j % 3]));

					glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(mvp_mat));
					mySphere.render();
				}
			}
		}

		// render ground plane
		{
			glm::mat4 model_matrix = glm::mat4(1.0);
			glm::mat4 mvp_mat = g_proj_matrix * g_view_matrix * model_matrix;

			glUniform3fv(ColorID, 1, glm::value_ptr(albedo));

			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(mvp_mat));
			myPlane.render();
		}

		// swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} while (glfwWindowShouldClose(window) == 0);

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
}
