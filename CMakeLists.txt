cmake_minimum_required(VERSION 3.20)

project(LeoVultana)

add_compile_options(/MP)

# Check MSVC toolset version, Visual Studio 2019 required
if(MSVC_TOOLSET_VERSION VERSION_LESS 142)
    message(FATAL_ERROR "Cannot find MSVC toolset version 142 or greater. Please make sure Visual Studio 2019 or newer installed")
endif()

# ouput exe to bin directory
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/Binaries)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_HOME_DIRECTORY}/Binaries )
endforeach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )

add_compile_options(/MP)

# reference libs used by both backends
include_directories(Externals)
include_directories(Externals/GLM)
include_directories(Externals/ImGui)
include_directories(Externals/TinyGLTF)
include_directories(Externals/Json)

if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
    message(STATUS "Using module to find Vulkan")
    find_package(Vulkan)
endif()

IF (NOT Vulkan_FOUND)
    find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/External/Vulkan/Lib)
    IF (Vulkan_LIBRARY)
        set(Vulkan_FOUND ON)
        MESSAGE("Using bundled Vulkan library version")
    ENDIF()
ENDIF()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")

# Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES /EHsc")

add_subdirectory(Common)
add_subdirectory(Projects)