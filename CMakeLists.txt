cmake_minimum_required(VERSION 3.24)
project(iiFileProvider VERSION 0.1.0 LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
        set(_default_home "$ENV{HOME}")
    elseif(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
        set(_default_home "$ENV{USERPROFILE}")
    else()
        message(FATAL_ERROR "Set CMAKE_INSTALL_PREFIX when HOME and USERPROFILE are unavailable")
    endif()
    set(CMAKE_INSTALL_PREFIX "${_default_home}/.local/SDK/iiFileProvider" CACHE PATH "Install prefix" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Core)

add_library(iiFileProvider SHARED iiFileProvider.h iiFileProvider.cpp)
add_library(iiFileProvider::iiFileProvider ALIAS iiFileProvider)
target_compile_features(iiFileProvider PUBLIC cxx_std_20)
target_compile_definitions(iiFileProvider PRIVATE IIFILEPREVIEW_BUILDING_LIBRARY)
target_link_libraries(iiFileProvider PUBLIC Qt6::Core)
target_include_directories(iiFileProvider PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(iiFileProvider PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

if(BUILD_TESTING)
    add_executable(iiFileProvider_hello_world tests/hello_world.cpp)
    target_link_libraries(iiFileProvider_hello_world PRIVATE iiFileProvider::iiFileProvider)
    set_target_properties(iiFileProvider_hello_world PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME iiFileProvider.hello_world COMMAND iiFileProvider_hello_world)
endif()

set(_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/iiFileProvider")
configure_package_config_file(
    cmake/iiFileProviderConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/iiFileProviderConfig.cmake"
    INSTALL_DESTINATION "${_package_dir}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/iiFileProviderConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(TARGETS iiFileProvider EXPORT iiFileProviderTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES iiFileProvider.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT iiFileProviderTargets NAMESPACE iiFileProvider:: DESTINATION "${_package_dir}")
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/iiFileProviderConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/iiFileProviderConfigVersion.cmake"
    DESTINATION "${_package_dir}"
)
install(FILES README.md DESTINATION "${CMAKE_INSTALL_DATADIR}/iiFileProvider")
