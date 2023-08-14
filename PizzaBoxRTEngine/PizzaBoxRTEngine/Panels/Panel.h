#pragma once
#if _WIN32
#include "../../External/imgui/imgui.h"
#include "../../External/glfw/include/GLFW/glfw3.h"
#else
#include "imgui.h"
#include <GLFW/glfw3.h>
#endif

namespace PBEngine
{
	class Panel
	{
	public:
		virtual void Show();
		virtual void Init();
		virtual ~Panel() {};
	};
}

