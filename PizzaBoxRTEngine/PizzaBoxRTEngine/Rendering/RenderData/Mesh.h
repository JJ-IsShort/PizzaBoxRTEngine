#pragma once

#include <glm/vec3.hpp>
#include <vector>

namespace PBEngine
{
	class Mesh
	{
	public:
		Mesh();
		Mesh(std::vector<glm::vec3>& newVertices);
		Mesh(std::vector<glm::vec3>& newVertices, std::vector<uint32_t>& newIndices);
		~Mesh();

	private:
		std::vector<uint32_t> indices;
		std::vector<glm::vec3> vertices;
	};
}