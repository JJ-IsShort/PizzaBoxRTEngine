#pragma once

#include "app.h"

PBEngine::App app;

int main()
{
    VkResult err = volkInitialize();
	if (err != VK_SUCCESS)
	{
		std::cout << "Volk initialisation failed" << "\n";
		return 0;
	}
    return app.Start();
}
