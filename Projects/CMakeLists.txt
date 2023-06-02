include(../CMake/Common.cmake)

file(GLOB_RECURSE ScenePath "${CMAKE_HOME_DIRECTORY}/Assets/Scenes/*.*")

function(buildProject PROJECT_NAME)
    SET(PROJECT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME})
    message(STATUS "Generating project file for project in ${PROJECT_FOLDER}")

    # Main
    file(GLOB_RECURSE PROJECT_SRC ${PROJECT_FOLDER}/*.cpp ${PROJECT_FOLDER}/*.h ProjectPCH.h)

    copyTargetCommand("${ScenePath}" ${CMAKE_HOME_DIRECTORY}/Binaries/ copied_common_config${PROJECT_NAME})

    add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SRC})
    add_dependencies(${PROJECT_NAME} copied_common_config${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} LINK_PUBLIC CommonLib LeoVultanaVK ${Vulkan_LIBRARY} ${WINLIBS})
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Binaries)

endfunction(buildProject)

# Build all examples
function(buildProjects)
    foreach(PROJECT ${PROJECTS})
        buildProject(${PROJECT})
    endforeach(PROJECT)
endfunction(buildProjects)

set(PROJECTS
#        GLTFSample
        LibraryTest
        )

buildProjects()
