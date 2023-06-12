include(${CMAKE_HOME_DIRECTORY}/CMake/Common.cmake)

#set(GLTFVK_SRC
#    GLTFRenderPasses/GLTFBBoxPassVK.h
#    GLTFRenderPasses/GLTFBBoxPassVK.cpp
#    GLTFRenderPasses/GLTFDepthPassVK.cpp
#    GLTFRenderPasses/GLTFDepthPassVK.h
#    GLTFRenderPasses/GLTFHelpersVK.cpp
#    GLTFRenderPasses/GLTFHelpersVK.h
#    GLTFRenderPasses/GLTFMotionVectorsPassVK.h
#    GLTFRenderPasses/GLTFMotionVectorsPassVK.cpp
#    GLTFRenderPasses/GLTFPBRPassVK.cpp
#    GLTFRenderPasses/GLTFPBRPassVK.h
#    GLTFRenderPasses/GLTFTexturesAndBuffersVK.h
#    GLTFRenderPasses/GLTFTexturesAndBuffersVK.cpp)
#
#set(BaseVK_SRC
#    CommandListRingVK.h CommandListRingVK.cpp
#    DevicePropertiesVK.h DevicePropertiesVK.cpp
#    DeviceVK.h DeviceVK.cpp
#    DynamicBufferRingVK.h DynamicBufferRingVK.cpp
#    ExtDebugUtilsVK.h ExtDebugUtilsVK.cpp
#    ExtFP16VK.h ExtFP16VK.cpp
#    ExtFreeSyncHDRVK.h ExtFreeSyncHDRVK.cpp
#    ExtRayTracingVK.h ExtRayTracingVK.cpp
#    ExtValidationVK.h ExtValidationVK.cpp
#    ExtVRSVK.h ExtVRSVK.cpp
#    FrameworkWindowsVK.h FrameworkWindowsVK.cpp
#    FreeSyncHDRVK.h FreeSyncHDRVK.cpp
#    GBufferVK.h GBufferVK.cpp
#    GPUTimeStampsVK.h GPUTimeStampsVK.cpp
#    HelperVK.h HelperVK.cpp
#    ImGuiVK.h ImGuiVK.cpp
#    InstancePropertiesVK.h InstancePropertiesVK.cpp
#    InstanceVK.h InstanceVK.cpp
#    ResourceViewHeapsVK.h ResourceViewHeapsVK.cpp
#    ShaderCompilerHelperVK.h ShaderCompilerHelperVK.cpp
#    StaticBufferPoolVK.h StaticBufferPoolVK.cpp
#    SwapChainVK.h SwapChainVK.cpp
#    TextureVK.h TextureVK.cpp
#    UploadHeapVK.h UploadHeapVK.cpp)
#
#set(PostProcess_SRC
#    PostProcess/MagnifierPS.h PostProcess/MagnifierPS.cpp
#    PostProcess/PostProcessPS.h PostProcess/PostProcessPS.cpp
#    PostProcess/SkyDome.h PostProcess/SkyDome.cpp)
#
#set(Widgets_SRC
#    Widgets/AxisVK.h Widgets/AxisVK.cpp
#    Widgets/CheckerBoardFloorVK.h Widgets/CheckerBoardFloorVK.cpp
#    Widgets/WireframeBoxVK.h Widgets/WireframeBoxVK.cpp
#    Widgets/WireframeSphereVK.h Widgets/WireframeSphereVK.cpp
#    Widgets/WireframeVK.h Widgets/WireframeVK.cpp)

file(GLOB_RECURSE LeoVultanaVK_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE LeoVultanaVK_HEAD "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

file(GLOB_RECURSE Shaders_GLTF_src "${CMAKE_HOME_DIRECTORY}/Assets/Shaders/GLSL/*.*")

add_library(LeoVultanaVK STATIC ${LeoVultanaVK_SRC} ${LeoVultanaVK_HEAD})

target_link_libraries(LeoVultanaVK CommonLib Vulkan::Vulkan)
target_include_directories (LeoVultanaVK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

copyTargetCommand("${Shaders_GLTF_src}" ${CMAKE_HOME_DIRECTORY}/Binaries/ShaderLibVK copied_vk_shaders_gltf_src)
#copyTargetCommand("${Shaders_PostProc_src}" ${CMAKE_HOME_DIRECTORY}/Binaries/ShaderLibVK copied_vk_shaders_postproc_src)
add_dependencies(LeoVultanaVK copied_vk_shaders_gltf_src)

set_source_files_properties(${Shaders_GLTF_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
#set_source_files_properties(${Shaders_PostProc_src} PROPERTIES VS_TOOL_OVERRIDE "Text")