#pragma once

#include "GLTFSamplePCH.h"
#include "GLTF/GLTFCommon.h"
#include "Renderer.h"
#include "UI.h"

// This class encapsulates the 'application' and is responsible for handling window events and scene updates (simulation)
// Rendering and rendering resource management is done by the Renderer class

class GLTFSample : public FrameworkWindows
{
public:
    GLTFSample(LPCSTR name);
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

    bool                        m_bIsBenchmarking;

    GLTFCommon                 *m_pGltfLoader = NULL;
    bool                        m_loadingScene = false;

    Renderer*                   m_pRenderer = NULL;
    UIState                     m_UIState;
    float                       m_fontSize;
    Camera                      m_camera;

    float                       m_time; // Time accumulator in seconds, used for animation.

    // json config file
    json                        m_jsonConfigFile;
    std::vector<std::string>    m_sceneNames;
    int                         m_activeScene;
    int                         m_activeCamera;

    bool                        m_bPlay;
};
