cmake_minimum_required(VERSION 3.24)
project(iiFileProviderInstalledConsumer LANGUAGES CXX)

find_package(iiFileProvider 0.1.0 CONFIG REQUIRED)
add_executable(iiFileProvider_installed_consumer ../hello_world.cpp)
target_link_libraries(iiFileProvider_installed_consumer PRIVATE iiFileProvider::iiFileProvider)
set_target_properties(iiFileProvider_installed_consumer PROPERTIES CXX_EXTENSIONS OFF)

enable_testing()
add_test(NAME iiFileProvider.installed_consumer COMMAND iiFileProvider_installed_consumer)
