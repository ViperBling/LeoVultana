file(GLOB_RECURSE Common_SRC "*.cpp" ".hpp" ".h")
file(GLOB_RECURSE Common_HEAD "*.hpp" "*.h")

add_library(CommonLib STATIC ${Common_SRC} ${Common_HEAD})

target_link_libraries(CommonLib PUBLIC NJSON )
target_link_directories(CommonLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

