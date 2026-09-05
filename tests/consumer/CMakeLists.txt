cmake_minimum_required(VERSION 3.24)
project(iiFilePreviewInstalledConsumer LANGUAGES CXX)

find_package(iiFilePreview 0.1.0 CONFIG REQUIRED)
add_executable(iiFilePreview_installed_consumer ../hello_world.cpp)
target_link_libraries(iiFilePreview_installed_consumer PRIVATE iiFilePreview::iiFilePreview)
set_target_properties(iiFilePreview_installed_consumer PROPERTIES CXX_EXTENSIONS OFF)

enable_testing()
add_test(NAME iiFilePreview.installed_consumer COMMAND iiFilePreview_installed_consumer)
