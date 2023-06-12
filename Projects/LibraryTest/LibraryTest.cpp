#include "LibraryTest.h"
#include "Utilities/DXCHelper.h"
#include "GLTF/GLTFHelpers.h"

#include "imgui.h"

// To use the 'disabled UI state' functionality (ImGuiItemFlags_Disabled), include internal header
// https://github.com/ocornut/imgui/issues/211#issuecomment-339241929
#include "imgui_internal.h"
#include "Utilities/Benchmark.h"

static void DisableUIStateBegin(const bool& bEnable)
{
    if (!bEnable)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
};
static void DisableUIStateEnd(const bool& bEnable)
{
    if (!bEnable)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
};

template<class T> static T clamped(const T& v, const T& fmin, const T& max)
{
    if (v < fmin)      return fmin;
    else if (v > max) return max;
    else              return v;
}

void LibraryTest::BuildUI()
{
    // if we haven't initialized GLTFLoader yet, don't draw UI.
    if (m_pGLTFLoader == nullptr)
    {
        LoadScene(mActiveScene);
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameBorderSize = 1.0f;

    const uint32_t W = this->GetWidth();
    const uint32_t H = this->GetHeight();

    const uint32_t PROFILER_WINDOW_PADDIG_X = 10;
    const uint32_t PROFILER_WINDOW_PADDIG_Y = 10;
    const uint32_t PROFILER_WINDOW_SIZE_X = 330;
    const uint32_t PROFILER_WINDOW_SIZE_Y = 450;
    const uint32_t PROFILER_WINDOW_POS_X = W - PROFILER_WINDOW_PADDIG_X - PROFILER_WINDOW_SIZE_X;
    const uint32_t PROFILER_WINDOW_POS_Y = PROFILER_WINDOW_PADDIG_Y;

    const uint32_t CONTROLS_WINDOW_POS_X = 10;
    const uint32_t CONTROLS_WINDOW_POS_Y = 10;
    const uint32_t CONTROLW_WINDOW_SIZE_X = 350;
    const uint32_t CONTROLW_WINDOW_SIZE_Y = 780; // assuming > 720p

    // Render CONTROLS window
    //
    ImGui::SetNextWindowPos(ImVec2(CONTROLS_WINDOW_POS_X, CONTROLS_WINDOW_POS_Y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(CONTROLW_WINDOW_SIZE_X, CONTROLW_WINDOW_SIZE_Y), ImGuiCond_FirstUseEver);

    if (mUIState.bShowControlsWindow)
    {
        ImGui::Begin("CONTROLS (F1)", &mUIState.bShowControlsWindow);
        if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Play", &m_bPlay);
            ImGui::SliderFloat("Time", &mTime, 0, 30);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char* cameraControl[] = { "Orbit", "WASD", "cam #0", "cam #1", "cam #2", "cam #3" , "cam #4", "cam #5" };

            if (mActiveCamera >= m_pGLTFLoader->mCameras.size() + 2)
                mActiveCamera = 0;
            ImGui::Combo("Camera", &mActiveCamera, cameraControl, fmin((int)(m_pGLTFLoader->mCameras.size() + 2), _countof(cameraControl)));

            auto getterLambda = [](void* data, int idx, const char** out_str)->bool { *out_str = ((std::vector<std::string> *)data)->at(idx).c_str(); return true; };
            if (ImGui::Combo("Model", &mActiveScene, getterLambda, &mSceneNames, (int)mSceneNames.size()))
            {
                LoadScene(mActiveScene);

                //bail out as we need to reload everything
                ImGui::End();
                ImGui::EndFrame();
                ImGui::NewFrame();
                return;
            }

            for (int i = 0; i < m_pGLTFLoader->mLights.size(); i++)
            {
                ImGui::SliderFloat(format("Light %i Intensity", i).c_str(), &m_pGLTFLoader->mLights[i].mIntensity, 0.0f, 50.0f);
            }
            if (ImGui::Button("Set Spot Light 0 to Camera's View"))
            {
                int idx = m_pGLTFLoader->mLightInstances[0].mNodeIndex;
                m_pGLTFLoader->mNodes[idx].mTransform.LookAt(mCamera.GetPosition(), mCamera.GetPosition() - mCamera.GetDirection());
                m_pGLTFLoader->mAnimatedMats[idx] = m_pGLTFLoader->mNodes[idx].mTransform.GetWorldMat();
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Presentation Mode", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* fullscreenModes[] = { "Windowed", "BorderlessFullscreen", "ExclusiveFulscreen" };
            if (ImGui::Combo("Fullscreen Mode", (int*)&mFullScreenMode, fullscreenModes, _countof(fullscreenModes)))
            {
                if (mPreviousFullScreenMode != mFullScreenMode)
                {
                    HandleFullScreen();
                    mPreviousFullScreenMode = mFullScreenMode;
                }
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (mFreeSyncHDROptionEnabled && ImGui::CollapsingHeader("FreeSync HDR", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static bool openWarning = false;
            const char** displayModeNames = &mDisplayModesNamesAvailable[0];
            if (ImGui::Combo("Display Mode", (int*)&mCurrentDisplayModeNamesIndex, displayModeNames, (int)mDisplayModesNamesAvailable.size()))
            {
                if (mFullScreenMode != PRESENTATIONMODE_WINDOWED)
                {
                    UpdateDisplay();
                    mPreviousDisplayModeNamesIndex = mCurrentDisplayModeNamesIndex;
                }
                else if (CheckIfWindowModeHDROn() &&
                         (mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DISPLAYMODE_SDR ||
                          mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_2084 ||
                          mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_SCRGB))
                {
                    UpdateDisplay();
                    mPreviousDisplayModeNamesIndex = mCurrentDisplayModeNamesIndex;
                }
                else
                {
                    openWarning = true;
                    mCurrentDisplayModeNamesIndex = mPreviousDisplayModeNamesIndex;
                }
            }

            if (openWarning)
            {
                ImGui::OpenPopup("Display Modes Warning");
                ImGui::BeginPopupModal("Display Modes Warning", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::Text("\nChanging display modes is only available either using HDR toggle in windows display setting for HDR10 modes or in fullscreen for FS HDR modes\n\n");
                if (ImGui::Button("Cancel", ImVec2(120, 0))) { openWarning = false; ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }

            if (mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_FSHDR_Gamma22 ||
                mDisplayModesAvailable[mCurrentDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_FSHDR_SCRGB)
            {
                if (ImGui::Checkbox("Enable Local Dimming", &mEnableLocalDimming))
                {
                    OnLocalDimmingChanged();
                }
            }
        }
        ImGui::End(); // CONTROLS
    }


    // Render PROFILER window
    if (mUIState.bShowProfilerWindow)
    {
        constexpr size_t NUM_FRAMES = 128;
        static float FRAME_TIME_ARRAY[NUM_FRAMES] = { 0 };

        // track highest frame rate and determine the max value of the graph based on the measured highest value
        static float RECENT_HIGHEST_FRAME_TIME = 0.0f;
        constexpr int FRAME_TIME_GRAPH_MAX_FPS[] = { 800, 240, 120, 90, 60, 45, 30, 15, 10, 5, 4, 3, 2, 1 };
        static float  FRAME_TIME_GRAPH_MAX_VALUES[_countof(FRAME_TIME_GRAPH_MAX_FPS)] = { 0 }; // us
        for (int i = 0; i < _countof(FRAME_TIME_GRAPH_MAX_FPS); ++i) { FRAME_TIME_GRAPH_MAX_VALUES[i] = 1000000.f / FRAME_TIME_GRAPH_MAX_FPS[i]; }

        //scrolling data and average FPS computing
        const std::vector<TimeStamp>& timeStamps = m_pRenderer->GetTimingValues();
        const bool bTimeStampsAvailable = timeStamps.size() > 0;
        if (bTimeStampsAvailable)
        {
            RECENT_HIGHEST_FRAME_TIME = 0;
            FRAME_TIME_ARRAY[NUM_FRAMES - 1] = timeStamps.back().mMicroseconds;
            for (uint32_t i = 0; i < NUM_FRAMES - 1; i++)
            {
                FRAME_TIME_ARRAY[i] = FRAME_TIME_ARRAY[i + 1];
            }
            RECENT_HIGHEST_FRAME_TIME = fmax(RECENT_HIGHEST_FRAME_TIME, FRAME_TIME_ARRAY[NUM_FRAMES - 1]);
        }
        const float& frameTime_us = FRAME_TIME_ARRAY[NUM_FRAMES - 1];
        const float  frameTime_ms = frameTime_us * 0.001f;
        const int fps = bTimeStampsAvailable ? static_cast<int>(1000000.0f / frameTime_us) : 0;

        // UI
        ImGui::SetNextWindowPos(ImVec2((float)PROFILER_WINDOW_POS_X, (float)PROFILER_WINDOW_POS_Y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(PROFILER_WINDOW_SIZE_X, PROFILER_WINDOW_SIZE_Y), ImGuiCond_FirstUseEver);
        ImGui::Begin("PROFILER (F2)", &mUIState.bShowProfilerWindow);

        ImGui::Text("Resolution : %ix%i", mWidth, mHeight);
        ImGui::Text("API        : %s", mSystemInfo.mGfxAPI.c_str());
        ImGui::Text("GPU        : %s", mSystemInfo.mGPUName.c_str());
        ImGui::Text("CPU        : %s", mSystemInfo.mCPUName.c_str());
        ImGui::Text("FPS        : %d (%.2f ms)", fps, frameTime_ms);

        if (ImGui::CollapsingHeader("GPU Timings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            std::string msOrUsButtonText = mUIState.bShowMilliseconds ? "Switch to microseconds(us)" : "Switch to milliseconds(ms)";
            if (ImGui::Button(msOrUsButtonText.c_str())) {
                mUIState.bShowMilliseconds = !mUIState.bShowMilliseconds;
            }
            ImGui::Spacing();

            if (mIsCPUValidationLayerEnabled || mIsGPUValidationLayerEnabled)
            {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "WARNING: Validation layer is switched on");
                ImGui::Text("Performance numbers may be inaccurate!");
            }

            // find the index of the FrameTimeGraphMaxValue as the next higher-than-recent-highest-frame-time in the pre-determined value list
            size_t iFrameTimeGraphMaxValue = 0;
            for (int i = 0; i < _countof(FRAME_TIME_GRAPH_MAX_VALUES); ++i)
            {
                if (RECENT_HIGHEST_FRAME_TIME < FRAME_TIME_GRAPH_MAX_VALUES[i]) // FRAME_TIME_GRAPH_MAX_VALUES are in increasing order
                {
                    iFrameTimeGraphMaxValue = fmin(_countof(FRAME_TIME_GRAPH_MAX_VALUES) - 1, i + 1);
                    break;
                }
            }
            ImGui::PlotLines("", FRAME_TIME_ARRAY, NUM_FRAMES, 0, "GPU frame time (us)", 0.0f, FRAME_TIME_GRAPH_MAX_VALUES[iFrameTimeGraphMaxValue], ImVec2(0, 80));

            for (uint32_t i = 0; i < timeStamps.size(); i++)
            {
                float value = mUIState.bShowMilliseconds ? timeStamps[i].mMicroseconds / 1000.0f : timeStamps[i].mMicroseconds;
                const char* pStrUnit = mUIState.bShowMilliseconds ? "ms" : "us";
                ImGui::Text("%-18s: %7.2f %s", timeStamps[i].mLabel.c_str(), value, pStrUnit);
            }
        }
        ImGui::End(); // PROFILER
    }
}

LibraryTest::LibraryTest(LPCSTR name) : FrameworkWindows(name)
{
    mTime = 0;
    m_bPlay = true;
    m_pGLTFLoader = nullptr;
}

void LibraryTest::OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight)
{
    // set some default values
    *pWidth = 1920;
    *pHeight = 1080;
    mActiveScene = 0;          //load the first one by default
    mVsyncEnabled = false;
    mFontSize = 13.f;
    mActiveCamera = 0;

    // read globals
    auto process = [&](json jData)
    {
        *pWidth = jData.value("width", *pWidth);
        *pHeight = jData.value("height", *pHeight);
        mFullScreenMode = jData.value("presentationMode", mFullScreenMode);
        mActiveScene = jData.value("activeScene", mActiveScene);
        mActiveCamera = jData.value("activeCamera", mActiveCamera);
        mIsCPUValidationLayerEnabled = jData.value("CpuValidationLayerEnabled", mIsCPUValidationLayerEnabled);
        mIsGPUValidationLayerEnabled = jData.value("GpuValidationLayerEnabled", mIsGPUValidationLayerEnabled);
        mVsyncEnabled = jData.value("vsync", mVsyncEnabled);
        mFontSize = jData.value("fontsize", mFontSize);
    };

    //read json globals from commandline
    try
    {
        if (strlen(lpCmdLine) > 0)
        {
            auto j3 = json::parse(lpCmdLine);
            process(j3);
        }
    }
    catch (json::parse_error)
    {
        Trace("Error parsing commandline\n");
        exit(0);
    }

    // read config file (and override values from commandline if so)
    {
        std::ifstream f("LibraryTest.json");
        if (!f)
        {
            MessageBox(nullptr, "Config file not found!\n", "Cauldron Panic!", MB_ICONERROR);
            exit(0);
        }

        try
        {
            f >> mJsonConfigFile;
        }
        catch (json::parse_error)
        {
            MessageBox(nullptr, "Error parsing LibraryTest.json!\n", "Cauldron Panic!", MB_ICONERROR);
            exit(0);
        }
    }

    json globals = mJsonConfigFile["globals"];
    process(globals);

    // get the list of scenes
    for (const auto & scene : mJsonConfigFile["scenes"])
        mSceneNames.push_back(scene["name"]);
}

void LibraryTest::OnCreate()
{
    // Init the shader compiler
    InitDirectXCompiler();
    CreateShaderCache();

    // Create a instance of the renderer and initialize it, we need to do that for each GPU
    m_pRenderer = new Renderer();
    m_pRenderer->OnCreate(&mDevice, &mSwapChain, mFontSize);

    // init GUI (non gfx stuff)
    ImGUI_Init((void *)mWindowHWND);
    mUIState.Initialize();

    OnResize(true);
    OnUpdateDisplay();

    // Init Camera, looking at the origin
    mCamera.LookAt(math::Vector4(0, 0, 5, 0), math::Vector4(0, 0, 0, 0));
}

void LibraryTest::OnDestroy()
{
    ImGUI_Shutdown();

    mDevice.GPUFlush();

    m_pRenderer->UnloadScene();
    m_pRenderer->OnDestroyWindowSizeDependentResources();
    m_pRenderer->OnDestroy();

    delete m_pRenderer;

    // shut down the shader compiler 
    DestroyShaderCache(&mDevice);

    if (m_pGLTFLoader)
    {
        delete m_pGLTFLoader;
        m_pGLTFLoader = nullptr;
    }
}

void LibraryTest::OnRender()
{
    // Do any start of frame necessities
    BeginFrame();

    ImGUI_UpdateIO();
    ImGui::NewFrame();

    if (m_bLoadScene)
    {
        // the scene loads in chuncks, that way we can show a progress bar
        static int loadingStage = 0;
        loadingStage = m_pRenderer->LoadScene(m_pGLTFLoader, loadingStage);
        if (loadingStage == 0)
        {
            mTime = 0;
            m_bLoadScene = false;
        }
    }
    else if (m_pGLTFLoader)
    {
        // Benchmarking takes control of the time, and exits the app when the animation is done
        std::vector<TimeStamp> timeStamps = m_pRenderer->GetTimingValues();
        std::string Filename;
    }
    else
    {
        BuildUI();  // UI logic. Note that the rendering of the UI happens later.
        OnUpdate(); // Update camera, handle keyboard/mouse input
    }

    // Do Render frame using AFR
    m_pRenderer->OnRender(&mUIState, mCamera, &mSwapChain);

    // Framework will handle Present and some other end of frame logic
    EndFrame();
}

bool LibraryTest::OnEvent(MSG msg)
{
    if (ImGUI_WndProcHandler(msg.hwnd, msg.message, msg.wParam, msg.lParam))
        return true;

    // handle function keys (F1, F2...) here, rest of the input is handled
    // by imGUI later in HandleInput() function
    const WPARAM& KeyPressed = msg.wParam;
    switch (msg.message)
    {
        case WM_KEYUP:
        case WM_SYSKEYUP:
            /* WINDOW TOGGLES */
            if (KeyPressed == VK_F1) mUIState.bShowControlsWindow ^= 1;
            if (KeyPressed == VK_F2) mUIState.bShowProfilerWindow ^= 1;
            break;
    }

    return true;
}

void LibraryTest::OnResize(bool resizeRender)
{
    // destroy resources (if we are not minimized)
    if (resizeRender && mWidth && mHeight && m_pRenderer)
    {
        m_pRenderer->OnDestroyWindowSizeDependentResources();
        m_pRenderer->OnCreateWindowSizeDependentResources(&mSwapChain, mWidth, mHeight);
    }

    mCamera.SetFov(AMD_PI_OVER_4, mWidth, mHeight, 0.1f, 1000.0f);
}

void LibraryTest::OnUpdateDisplay()
{
    // Destroy resources (if we are not minimized)
    if (m_pRenderer)
    {
        m_pRenderer->OnUpdateDisplayDependentResources(&mSwapChain);
    }
}

void LibraryTest::LoadScene(int sceneIndex)
{
    json scene = mJsonConfigFile["scenes"][sceneIndex];

    // release everything and load the GLTF, just the light json data, the rest (textures and geometry) will be done in the main loop
    if (m_pGLTFLoader != nullptr)
    {
        m_pRenderer->UnloadScene();
        m_pRenderer->OnDestroyWindowSizeDependentResources();
        m_pRenderer->OnDestroy();
        m_pGLTFLoader->Unload();
        m_pRenderer->OnCreate(&mDevice, &mSwapChain, mFontSize);
        m_pRenderer->OnCreateWindowSizeDependentResources(&mSwapChain, mWidth, mHeight);
    }

    delete(m_pGLTFLoader);
    m_pGLTFLoader = new GLTFCommon();
    if (!m_pGLTFLoader->Load(scene["directory"], scene["filename"]))
    {
        MessageBox(nullptr, "The selected model couldn't be found, please check the documentation", "Cauldron Panic!", MB_ICONERROR);
        exit(0);
    }

    // Load the UI settings, and also some defaults cameras and lights, in case the GLTF has none
    {
        // Add a default light in case there are none
        if (m_pGLTFLoader->mLights.empty())
        {
            gltfNode node;
            node.mTransform.LookAt(PolarToVector(AMD_PI_OVER_2, 0.58f) * 3.5f, math::Vector4(0, 0, 0, 0));

            gltfLight light;
            light.mType = gltfLight::LIGHT_SPOTLIGHT;
            light.mIntensity = scene.value("intensity", 1.0f);
            light.mColor = math::Vector4(1.0f, 1.0f, 1.0f, 0.0f);
            light.mRange = 15;
            light.mOuterConeAngle = AMD_PI_OVER_4;
            light.mInnerConeAngle = AMD_PI_OVER_4 * 0.9f;
            light.mShadowResolution = 1024;

            m_pGLTFLoader->AddLight(node, light);
        }

        // Allocate shadow information (if any)
        m_pRenderer->AllocateShadowMaps(m_pGLTFLoader);

        // set default camera
        json camera = scene["camera"];
        mActiveCamera = scene.value("activeCamera", mActiveCamera);
        math::Vector4 from = GetVector(GetElementJsonArray(camera, "defaultFrom", { 0.0, 0.0, 10.0 }));
        math::Vector4 to = GetVector(GetElementJsonArray(camera, "defaultTo", { 0.0, 0.0, 0.0 }));
        mCamera.LookAt(from, to);

        // indicate the mainloop we started loading a GLTF and it needs to load the rest (textures and geometry)
        m_bLoadScene = true;
    }
}

void LibraryTest::OnUpdate()
{
    ImGuiIO& io = ImGui::GetIO();

    //If the mouse was not used by the GUI then it's for the camera
    if (io.WantCaptureMouse)
    {
        io.MouseDelta.x = 0;
        io.MouseDelta.y = 0;
        io.MouseWheel = 0;
    }

    // Update Camera
    UpdateCamera(mCamera, io);
    mCamera.SetProjectionJitter(0.f, 0.f);

    // Keyboard & Mouse
    HandleInput(io);

    // Animation Update
    if (m_bPlay)
        mTime += (float)mDeltaTime / 1000.0f; // animation time in seconds

    if (m_pGLTFLoader)
    {
        m_pGLTFLoader->SetAnimationTime(0, mTime);
        m_pGLTFLoader->TransformScene(0, math::Matrix4::identity());
    }
}

void LibraryTest::HandleInput(const ImGuiIO& io)
{
    auto fnIsKeyTriggered = [&io](char key) { return io.KeysDown[key] && io.KeysDownDuration[key] == 0.0f; };
}

void LibraryTest::UpdateCamera(Camera& cam, const ImGuiIO& io)
{
    float yaw = cam.GetYaw();
    float pitch = cam.GetPitch();
    float distance = cam.GetDistance();

    cam.UpdatePreviousMatrices(); // set previous view matrix

    // Sets Camera based on UI selection (WASD, Orbit or any of the GLTF cameras)
    if (!io.KeyCtrl && io.MouseDown[0])
    {
        yaw -= io.MouseDelta.x / 100.f;
        pitch += io.MouseDelta.y / 100.f;
    }

    // Choose camera movement depending on setting
    if (mActiveCamera == 0)
    {
        // If nothing has changed, don't calculate an update (we are getting micro changes in view causing bugs)
        if (!io.MouseWheel && (!io.MouseDown[0] || (!io.MouseDelta.x && !io.MouseDelta.y)))
            return;

        //  Orbiting
        distance -= (float)io.MouseWheel / 3.0f;
        distance = std::max<float>(distance, 0.1f);

        bool panning = io.KeyCtrl && io.MouseDown[0];

        cam.UpdateCameraPolar(yaw, pitch,
                              panning ? -io.MouseDelta.x / 100.0f : 0.0f,
                              panning ? io.MouseDelta.y / 100.0f : 0.0f,
                              distance);
    }
    else if (mActiveCamera == 1)
    {
        //  WASD
        cam.UpdateCameraWASD(yaw, pitch, io.KeysDown, io.DeltaTime);
    }
    else if (mActiveCamera > 1)
    {
        // Use a camera from the GLTF
        m_pGLTFLoader->GetCamera(mActiveCamera - 2, &cam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    LPCSTR Name = "SampleVK v1.4.1";

    // create new Vulkan sample
    return RunFramework(hInstance, lpCmdLine, nCmdShow, new LibraryTest(Name));
}