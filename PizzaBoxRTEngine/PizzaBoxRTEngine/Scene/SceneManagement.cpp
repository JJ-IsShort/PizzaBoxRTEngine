#include "SceneManagement.h"

namespace PBEngine
{
	std::vector<std::shared_ptr<Scene>> SceneManager::scenes;
	int SceneManager::currentSceneIndex = -1;

	void SceneManager::Construct()
	{
		scenes.push_back(std::make_shared<Scene>());
		currentSceneIndex = 0;
	}

	SceneManager::~SceneManager()
	{

	}
	
	/*Scene& SceneManager::GetCurrentRef()
	{
		if (currentSceneIndex >= 0 && currentSceneIndex < scenes.size()) {
			return (scenes[currentSceneIndex].);
		}
		return nullptr;
	}*/
	
	std::shared_ptr<Scene> SceneManager::GetCurrent()
	{
		if (SceneManager::currentSceneIndex >= 0 && SceneManager::currentSceneIndex < scenes.size()) {
			return SceneManager::scenes[currentSceneIndex];
		}
		return nullptr;
	}
}