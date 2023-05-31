#pragma once

#include "RHI/Vulkan/PostProcess/MagnifierPS.h"
#include <string>

using namespace LeoVultana_VK;

struct UIState
{
    //
    // WINDOW MANAGEMENT
    //
    bool bShowControlsWindow;
    bool bShowProfilerWindow;


    //
    // POST PROCESS CONTROLS
    //
    int   SelectedTonemapperIndex;
    float Exposure;

    bool  bUseTAA;

    bool  bUseMagnifier;
    bool  bLockMagnifierPosition;
    bool  bLockMagnifierPositionHistory;
    int   LockedMagnifiedScreenPositionX;
    int   LockedMagnifiedScreenPositionY;
    MagnifierPS::PassParameters MagnifierParams;

    //
    // APP/SCENE CONTROLS
    //
    float IBLFactor;
    float EmissiveFactor;
    int   SelectedSkydomeTypeIndex;

    bool  bDrawLightFrustum;
    bool  bDrawBoundingBoxes;

    enum class WireframeMode : int
    {
        WIREFRAME_MODE_OFF = 0,
        WIREFRAME_MODE_SHADED = 1,
        WIREFRAME_MODE_SOLID_COLOR = 2,
    };

    WireframeMode WireframeMode;
    float         WireframeColor[3];

    //
    // PROFILER CONTROLS
    //
    bool  bShowMilliseconds;

    // -----------------------------------------------

    void Initialize();

    void ToggleMagnifierLock();
    void AdjustMagnifierSize(float increment = 0.05f);
    void AdjustMagnifierMagnification(float increment = 1.00f);
};