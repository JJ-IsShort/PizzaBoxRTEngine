#include "Mesh.h"

namespace PBEngine
{
	Mesh::Mesh()
	{
		vertices.push_back({ -0.5f, -0.5f, 0.0f });
		vertices.push_back({  0.5f, -0.5f, 0.0f });
		vertices.push_back({  0.0f,  0.5f, 0.0f });
		indices = { 0, 1, 2 };
	}

	Mesh::Mesh(std::vector<glm::vec3>& newVertices)
	{
		vertices = newVertices;
		for (size_t i = 0; i < newVertices.size(); i++)
		{
			indices.push_back(i);
		}
	}
	
	Mesh::Mesh(std::vector<glm::vec3>& newVertices, std::vector<uint32_t>& newIndices)
	{
		vertices = newVertices;
		indices = newIndices;
	}

	Mesh::~Mesh()
	{

	}
}