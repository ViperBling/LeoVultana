file(GLOB_RECURSE Common_SRC
    "Function/*.cpp"
    "GLTF/*.cpp"
    "Utilities/*.cpp"
    "../Externals/ImGui/*.cpp")

file(GLOB_RECURSE Common_HEAD
    "Function/.h"
    "GLTF/*.h"
    "Utilities/*.h")

include_directories(Function)
include_directories(GLTF)
include_directories(RHI)
include_directories(Utilities)

add_subdirectory(RHI/Vulkan)
#add_subdirectory(RHI/D3D12)

add_library(CommonLib STATIC ${Common_SRC} ${Common_HEAD})

#target_link_libraries(CommonLib PUBLIC user32 d3d11 d3dcompiler dxgi)
target_link_libraries(CommonLib PUBLIC user32 d3d11 d3dcompiler dxgi)

#target_link_directories(CommonLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})