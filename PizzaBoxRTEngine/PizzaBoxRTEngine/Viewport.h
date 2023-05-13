#pragma once
#include "Panel.h"
#include "Renderer.h"

class Viewport :
    public Panel 
{
public:
    Viewport();
    void Show() override;
    Renderer renderer;
};

