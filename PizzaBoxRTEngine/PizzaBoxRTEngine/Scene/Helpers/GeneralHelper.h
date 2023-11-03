#pragma once

#include <Scene/Scene.h>
#include <memory>

namespace PBEngine::SceneHelpers
{
	template <typename T>
	static void RunIfDerivedAs(auto argument, auto call)
	{
		T derived = dynamic_cast<T>(argument);
		if (derived)
		{
			call(derived);
		}
	}
	
	template <typename T>
	static void RunIfDerivedPointerAs(auto argument, auto call)
	{
		std::shared_ptr<T> derived = std::dynamic_pointer_cast<T>(argument);
		if (derived)
		{
			call(derived);
		}
	}
}