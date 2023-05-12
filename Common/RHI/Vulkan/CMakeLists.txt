file(GLOB_RECURSE LeoVultanaVK_SRC "*.cpp")
file(GLOB_RECURSE LeoVultanaVK_HEAD "*.h")

add_library(LeoVultanaVK STATIC ${LeoVultanaVK_SRC} ${LeoVultanaVK_HEAD})

target_link_libraries(LeoVultanaVK CommonLib Vulkan::Vulkan)
target_include_directories (LeoVultanaVK PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})