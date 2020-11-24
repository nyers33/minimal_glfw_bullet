#ifndef GLMESHDATA_H
#define GLMESHDATA_H

// Include GLEW
#include <GL/glew.h>

#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#define CHECK_GL { GLenum glStatus = glGetError(); if( glStatus != GL_NO_ERROR ) { std::cout << "File: " << __FILE__ << " " << "Line: " << __LINE__ << " " << "OpenGL error: " << openglGetErrorString( glStatus ); } }
static std::string openglGetErrorString(GLenum status)
{
	std::stringstream ss;

	switch (status)
	{
	case GL_INVALID_ENUM:
		ss << "GL_INVALID_ENUM";
		break;
	case GL_INVALID_VALUE:
		ss << "GL_INVALID_VALUE";
		break;
	case GL_INVALID_OPERATION:
		ss << "GL_INVALID_OPERATION";
		break;
	case GL_OUT_OF_MEMORY:
		ss << "GL_OUT_OF_MEMORY";
		break;
	default:
		ss << "GL_UNKNOWN_ERROR" << " - " << status;
		break;
	}

	return ss.str();
}

class GLMeshData
{
public:
	GLMeshData();
	~GLMeshData();

	void createBox(float w, float h, float l);
	void createPlane(float base, float size, float uvScale = 1.0f);
	void createSphere(float rad, uint32_t hSegs, uint32_t vSegs);

	void render();
	void clear();

protected:
	void createGLObjects();

	GLenum primitiveType;
	unsigned int numVertices;
	unsigned int numPrimitives;

	GLuint meshVAID;
	GLuint meshIBID;
	GLuint meshVBID_pos;
	GLuint meshVBID_uv;
	
	std::vector<GLuint> indexData;
	std::vector<GLfloat> posData;
	std::vector<GLfloat> uvData;
};

#endif