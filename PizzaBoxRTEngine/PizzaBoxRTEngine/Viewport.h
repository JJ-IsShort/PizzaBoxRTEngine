#pragma once
#include "Panel.h"

class Viewport :
    public Panel 
{
public:
    Viewport();
    void Show() override;
};

