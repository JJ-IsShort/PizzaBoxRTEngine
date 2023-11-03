#pragma once

#include <vector>
#include "RenderData/AccelerationStructure.h"

namespace PBEngine
{
	static class RenderInterface
	{
	public:
		RenderInterface();
		~RenderInterface();

		class RenderObject
		{
		public:
			RenderObject();
			~RenderObject();

			AccelerationStructure* accelStructure;
		};

		static std::vector<RenderObject>& GetRenderObjects() { return objects; };

	private:
		static std::vector<RenderObject> objects;
	};
}