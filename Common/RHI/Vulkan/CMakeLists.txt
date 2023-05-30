include(${CMAKE_HOME_DIRECTORY}/CMake/Common.cmake)

file(GLOB_RECURSE LeoVultanaVK_SRC "*.cpp")
file(GLOB_RECURSE LeoVultanaVK_HEAD "*.h")

file(GLOB_RECURSE Shaders_GLTF_src "${CMAKE_HOME_DIRECTORY}/Assets/Shaders/GLSL/*.*")

add_library(LeoVultanaVK STATIC ${LeoVultanaVK_SRC} ${LeoVultanaVK_HEAD})

target_link_libraries(LeoVultanaVK CommonLib Vulkan::Vulkan)
target_include_directories (LeoVultanaVK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

copyTargetCommand("${Shaders_GLTF_src}" ${CMAKE_HOME_DIRECTORY}/Binaries/ShaderLibVK copied_vk_shaders_gltf_src)
#copyTargetCommand("${Shaders_PostProc_src}" ${CMAKE_HOME_DIRECTORY}/Binaries/ShaderLibVK copied_vk_shaders_postproc_src)
add_dependencies(LeoVultanaVK copied_vk_shaders_gltf_src)

set_source_files_properties(${Shaders_GLTF_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
#set_source_files_properties(${Shaders_PostProc_src} PROPERTIES VS_TOOL_OVERRIDE "Text")