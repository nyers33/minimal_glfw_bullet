#include "glmeshdata.h"

GLMeshData::GLMeshData()
{
	meshVAID = meshVBID_pos = meshVBID_uv = meshIBID = 0;

	numVertices = numPrimitives = 0;

	primitiveType = GL_TRIANGLES;
}

GLMeshData::~GLMeshData()
{
	clear();
}

void GLMeshData::clear()
{
	if (meshVBID_pos)
	{
		glDeleteBuffers(1, &meshVBID_pos);
		meshVBID_pos = 0;
	}

	if (meshVBID_uv)
	{
		glDeleteBuffers(1, &meshVBID_uv);
		meshVBID_uv = 0;
	}

	if (meshIBID)
	{
		glDeleteBuffers(1, &meshIBID);
		meshIBID = 0;
	}

	if (meshVAID)
	{
		glDeleteVertexArrays(1, &meshVAID);
		meshVAID = 0;
	}
}

void GLMeshData::createPlane(float base, float size, float uvScale)
{
	numPrimitives = 2;

	posData = {
		-size,base,-size,
		 size,base,-size,
		 size,base, size,
		-size,base, size,
	};

	uvData = {
		0.0f*uvScale, 0.0f*uvScale,
		1.0f*uvScale, 0.0f*uvScale,
		1.0f*uvScale, 1.0f*uvScale,
		0.0f*uvScale, 1.0f*uvScale,
	};

	indexData = {
		0, 1, 2,
		2, 3, 0
	};

	createGLObjects();
}

void GLMeshData::createBox(float w, float h, float l)
{
	numPrimitives = 6*2;

	w *= 0.5f;
	h *= 0.5f;
	l *= 0.5f;

	// bottom face
	posData.insert(posData.end(), { w, -h,  l}); uvData.insert(uvData.end(), {0.0f, 0.0f});
	posData.insert(posData.end(), {-w, -h,  l}); uvData.insert(uvData.end(), {1.0f, 0.0f});
	posData.insert(posData.end(), {-w, -h, -l}); uvData.insert(uvData.end(), {1.0f, 1.0f});
	posData.insert(posData.end(), { w, -h, -l}); uvData.insert(uvData.end(), {0.0f, 1.0f});

	// top face
	posData.insert(posData.end(), {-w,  h,  l}); uvData.insert(uvData.end(), {0.0f, 0.0f});
	posData.insert(posData.end(), { w,  h,  l}); uvData.insert(uvData.end(), {1.0f, 0.0f});
	posData.insert(posData.end(), { w,  h, -l}); uvData.insert(uvData.end(), {1.0f, 1.0f});
	posData.insert(posData.end(), {-w,  h, -l}); uvData.insert(uvData.end(), {0.0f, 1.0f});

	// left face
	posData.insert(posData.end(), {-w, -h, -l}); uvData.insert(uvData.end(), {0.0f, 0.0f});
	posData.insert(posData.end(), {-w, -h,  l}); uvData.insert(uvData.end(), {1.0f, 0.0f});
	posData.insert(posData.end(), {-w,  h,  l}); uvData.insert(uvData.end(), {1.0f, 1.0f});
	posData.insert(posData.end(), {-w,  h, -l}); uvData.insert(uvData.end(), {0.0f, 1.0f});

	// right face
	posData.insert(posData.end(), { w, -h,  l}); uvData.insert(uvData.end(), {0.0f, 0.0f});
	posData.insert(posData.end(), { w, -h, -l}); uvData.insert(uvData.end(), {1.0f, 0.0f});
	posData.insert(posData.end(), { w,  h, -l}); uvData.insert(uvData.end(), {1.0f, 1.0f});
	posData.insert(posData.end(), { w,  h,  l}); uvData.insert(uvData.end(), {0.0f, 1.0f});

	// front face
	posData.insert(posData.end(), {-w, -h,  l}); uvData.insert(uvData.end(), {0.0f, 0.0f});
	posData.insert(posData.end(), { w, -h,  l}); uvData.insert(uvData.end(), {1.0f, 0.0f});
	posData.insert(posData.end(), { w,  h,  l}); uvData.insert(uvData.end(), {1.0f, 1.0f});
	posData.insert(posData.end(), {-w,  h,  l}); uvData.insert(uvData.end(), {0.0f, 1.0f});

	// back face
	posData.insert(posData.end(), { w, -h, -l}); uvData.insert(uvData.end(), {0.0f, 0.0f});
	posData.insert(posData.end(), {-w, -h, -l}); uvData.insert(uvData.end(), {1.0f, 0.0f});
	posData.insert(posData.end(), {-w,  h, -l}); uvData.insert(uvData.end(), {1.0f, 1.0f});
	posData.insert(posData.end(), { w,  h, -l}); uvData.insert(uvData.end(), {0.0f, 1.0f});

	std::vector< unsigned int > tri00;
	std::vector< unsigned int > tri01;
	tri00.push_back(0); tri00.push_back(1); tri00.push_back(2);
	tri01.push_back(0); tri01.push_back(2); tri01.push_back(3);

	std::vector< unsigned int > tri02;
	std::vector< unsigned int > tri03;
	tri02.push_back(4); tri02.push_back(5); tri02.push_back(6);
	tri03.push_back(4); tri03.push_back(6); tri03.push_back(7);

	std::vector< unsigned int > tri04;
	std::vector< unsigned int > tri05;
	tri04.push_back(8); tri04.push_back(9); tri04.push_back(10);
	tri05.push_back(8); tri05.push_back(10); tri05.push_back(11);

	std::vector< unsigned int > tri06;
	std::vector< unsigned int > tri07;
	tri06.push_back(12); tri06.push_back(13); tri06.push_back(14);
	tri07.push_back(12); tri07.push_back(14); tri07.push_back(15);

	std::vector< unsigned int > tri08;
	std::vector< unsigned int > tri09;
	tri08.push_back(16); tri08.push_back(17); tri08.push_back(18);
	tri09.push_back(16); tri09.push_back(18); tri09.push_back(19);

	std::vector< unsigned int > tri10;
	std::vector< unsigned int > tri11;
	tri10.push_back(20); tri10.push_back(21); tri10.push_back(22);
	tri11.push_back(20); tri11.push_back(22); tri11.push_back(23);

	indexData.insert(indexData.end(), tri00.begin(), tri00.end());
	indexData.insert(indexData.end(), tri01.begin(), tri01.end());
	indexData.insert(indexData.end(), tri02.begin(), tri02.end());
	indexData.insert(indexData.end(), tri03.begin(), tri03.end());
	indexData.insert(indexData.end(), tri04.begin(), tri04.end());
	indexData.insert(indexData.end(), tri05.begin(), tri05.end());
	indexData.insert(indexData.end(), tri06.begin(), tri06.end());
	indexData.insert(indexData.end(), tri07.begin(), tri07.end());
	indexData.insert(indexData.end(), tri08.begin(), tri08.end());
	indexData.insert(indexData.end(), tri09.begin(), tri09.end());
	indexData.insert(indexData.end(), tri10.begin(), tri10.end());
	indexData.insert(indexData.end(), tri11.begin(), tri11.end());

	createGLObjects();
}

void GLMeshData::createSphere(float rad, uint32_t hSegs, uint32_t vSegs)
{
	numPrimitives = hSegs * vSegs * 2;

	float dphi = (float)(2.0*M_PI) / (float)(hSegs);
	float dtheta = (float)(M_PI) / (float)(vSegs);

	for (uint32_t v = 0; v <= vSegs; ++v)
	{
		float theta = v * dtheta;

		for (uint32_t h = 0; h <= hSegs; ++h)
		{
			float phi = h * dphi;

			float x = std::sin(theta) * std::cos(phi);
			float y = std::cos(theta);
			float z = std::sin(theta) * std::sin(phi);

			posData.insert(posData.end(), { rad * x, rad * y, rad * z });
			uvData.insert(uvData.end(), { 1.0f - (float)h / hSegs, (float)v / vSegs });
		}
	}

	for (uint32_t v = 0; v < vSegs; v++)
	{
		for (uint32_t h = 0; h < hSegs; h++)
		{
			uint32_t topRight = v * (hSegs + 1) + h;
			uint32_t topLeft = v * (hSegs + 1) + h + 1;
			uint32_t lowerRight = (v + 1) * (hSegs + 1) + h;
			uint32_t lowerLeft = (v + 1) * (hSegs + 1) + h + 1;

			std::vector< unsigned int > tri0;
			std::vector< unsigned int > tri1;

			tri0.push_back(lowerLeft);
			tri0.push_back(lowerRight);
			tri0.push_back(topRight);

			tri1.push_back(lowerLeft);
			tri1.push_back(topRight);
			tri1.push_back(topLeft);

			indexData.insert(indexData.end(), tri0.begin(), tri0.end());
			indexData.insert(indexData.end(), tri1.begin(), tri1.end());
		}
	}

	createGLObjects();
}

void GLMeshData::createGLObjects()
{
	// create vertex buffer objects for pos, uv
	glGenBuffers(1, &meshVBID_pos);
	glBindBuffer(GL_ARRAY_BUFFER, meshVBID_pos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * posData.size(), posData.data(), GL_STATIC_DRAW);
	CHECK_GL;

	glGenBuffers(1, &meshVBID_uv);
	glBindBuffer(GL_ARRAY_BUFFER, meshVBID_uv);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * uvData.size(), uvData.data(), GL_STATIC_DRAW);
	CHECK_GL;

	// create index buffer object
	glGenBuffers(1, &meshIBID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIBID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indexData.size(), indexData.data(), GL_STATIC_DRAW);
	CHECK_GL;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	CHECK_GL;

	// create vertex array
	glGenVertexArrays(1, &meshVAID);
	glBindVertexArray(meshVAID);
	CHECK_GL;

	GLuint loc_pos = 0;
	GLuint los_uv = 1;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshIBID);

	glBindBuffer(GL_ARRAY_BUFFER, meshVBID_pos);
	glVertexAttribPointer(loc_pos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, meshVBID_uv);
	glVertexAttribPointer(los_uv, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	CHECK_GL;

	glEnableVertexAttribArray(loc_pos);
	glEnableVertexAttribArray(los_uv);
	CHECK_GL;

	glBindVertexArray(0);

	glDisableVertexAttribArray(loc_pos);
	glDisableVertexAttribArray(los_uv);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	CHECK_GL;
}

void GLMeshData::render()
{
	glBindVertexArray(meshVAID);
	CHECK_GL;

	glDrawElements(primitiveType, 3 * numPrimitives, GL_UNSIGNED_INT, (void*)0);
	CHECK_GL;

	glBindVertexArray(0);
	CHECK_GL;
}