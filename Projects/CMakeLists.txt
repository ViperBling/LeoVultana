function(buildProject PROJECT_NAME)
    SET(PROJECT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME})
    message(STATUS "Generating project file for project in ${PROJECT_FOLDER}")

    # Main
    file(GLOB_RECURSE PROJECT_SRC *.cpp ${Common_HEAD} ${PROJECT_FOLDER}/*.cpp)

    add_executable(${PROJECT_NAME} ${PROJECT_SRC})
    target_link_libraries(${PROJECT_NAME} CommonLib ${Vulkan_LIBRARY} ${WINLIBS})
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Binaries)

endfunction(buildProject)

# Build all examples
function(buildProjects)
    foreach(PROJECT ${PROJECTS})
        buildProject(${PROJECT})
    endforeach(PROJECT)
endfunction(buildProjects)

set(PROJECTS
        GLTFSample
        )

buildProjects()
