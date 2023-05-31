include(../CMake/Common.cmake)

#set(Base_SRC
#    Utilities/Sequence.h Utilities/Sequence.cpp
#    Utilities/Benchmark.h Utilities/Benchmark.h
#    Utilities/ImGuiHelper.h Utilities/ImGuiHelper.cpp)
#
#set(ShaderCompiler_SRC
#    Utilities/ShaderCompiler.h Utilities/ShaderCompiler.cpp
#    Utilities/ShaderCompilerCache.h Utilities/ShaderCompilerCache.cpp
#    Utilities/DXCHelper.h Utilities/DXCHelper.cpp)
#
#set(GLTF_SRC
#    GLTF/GLTFStructures.h
#    GLTF/GLTFCommon.h GLTF/GLTFCommon.cpp
#    GLTF/GLTFPBRMaterial.h GLTF/GLTFPBRMaterial.cpp
#    GLTF/GLTFHelpers.h GLTF/GLTFHelpers.cpp)

file(GLOB_RECURSE Common_SRC
    "Function/*.cpp"
    "GLTF/*.cpp"
    "Utilities/*.cpp"
    "../Externals/ImGui/*.cpp")

file(GLOB_RECURSE Common_HEAD
    "Function/*.h"
    "GLTF/*.h"
    "Utilities/*.h")

include_directories(Function)
include_directories(GLTF)
include_directories(RHI)
include_directories(Utilities)

add_subdirectory(RHI/Vulkan)

add_library(CommonLib STATIC ${Common_SRC} ${Common_HEAD})

target_link_libraries(CommonLib PUBLIC user32 d3d11 d3dcompiler dxgi)
target_include_directories(CommonLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})