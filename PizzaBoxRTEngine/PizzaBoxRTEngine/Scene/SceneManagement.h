#pragma once

#include <memory>
#include <Scene/Scene.h>

namespace PBEngine
{
	static class SceneManager
	{
	public:
		static void Construct();
		~SceneManager();

		//static Scene& GetCurrentRef();
		static std::shared_ptr<Scene> GetCurrent();
		
	private:
		static std::vector<std::shared_ptr<Scene>> scenes;
		static int currentSceneIndex;
	};
}