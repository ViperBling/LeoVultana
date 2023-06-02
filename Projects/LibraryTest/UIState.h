#pragma once

#include "ProjectPCH.h"

class UIState
{
public:
    void Initialize();
public:
    // WINDOW MANAGEMENT
    bool bShowControlsWindow;
    bool bShowProfilerWindow;

    // APP/SCENE CONTROLS
    bool  bDrawLightFrustum;

    // PROFILER CONTROLS
    bool  bShowMilliseconds;
};