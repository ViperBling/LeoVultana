#pragma once

#include "ProjectPCH.h"
#include "GLTF/GLTFCommon.h"
#include "Renderer.h"
#include "UIState.h"

// This class encapsulates the 'application' and is responsible for handling window events and scene updates (simulation)
// Rendering and rendering resource management is done by the Renderer class

class LibraryTest : public FrameworkWindows
{
public:
    LibraryTest(LPCSTR name);
    void OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight) override;
    void OnCreate() override;
    void OnDestroy() override;
    void OnRender() override;
    bool OnEvent(MSG msg) override;
    void OnResize(bool resizeRender) override;
    void OnUpdateDisplay() override;

    void BuildUI();
    void LoadScene(int sceneIndex);

    void OnUpdate();

    void HandleInput(const ImGuiIO& io);
    void UpdateCamera(Camera& cam, const ImGuiIO& io);

private:

    GLTFCommon*                 m_pGLTFLoader = nullptr;
    bool                        m_bLoadScene = false;
    Renderer*                   m_pRenderer = nullptr;
    UIState                     mUIState;

    float                       mFontSize;
    Camera                      mCamera;

    float                       mTime; // Time accumulator in seconds, used for animation.

    // json config file
    json                        mJsonConfigFile;
    std::vector<std::string>    mSceneNames;
    int                         mActiveScene;
    int                         mActiveCamera;

    bool                        m_bPlay;
};
