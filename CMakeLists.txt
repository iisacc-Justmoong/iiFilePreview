cmake_minimum_required(VERSION 3.24)
project(iiFilePreview VERSION 0.1.0 LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
        set(_default_home "$ENV{HOME}")
    elseif(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
        set(_default_home "$ENV{USERPROFILE}")
    else()
        message(FATAL_ERROR "Set CMAKE_INSTALL_PREFIX when HOME and USERPROFILE are unavailable")
    endif()
    set(CMAKE_INSTALL_PREFIX "${_default_home}/.local/SDK/iiFilePreview" CACHE PATH "Install prefix" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)
find_package(Qt6 6.8.3 EXACT REQUIRED COMPONENTS Core)

add_library(iiFilePreview SHARED iiFilePreview.h iiFilePreview.cpp)
add_library(iiFilePreview::iiFilePreview ALIAS iiFilePreview)
target_compile_features(iiFilePreview PUBLIC cxx_std_20)
target_compile_definitions(iiFilePreview PRIVATE IIFILEPREVIEW_BUILDING_LIBRARY)
target_link_libraries(iiFilePreview PUBLIC Qt6::Core)
target_include_directories(iiFilePreview PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(iiFilePreview PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

if(BUILD_TESTING)
    add_executable(iiFilePreview_hello_world tests/hello_world.cpp)
    target_link_libraries(iiFilePreview_hello_world PRIVATE iiFilePreview::iiFilePreview)
    set_target_properties(iiFilePreview_hello_world PROPERTIES CXX_EXTENSIONS OFF)
    add_test(NAME iiFilePreview.hello_world COMMAND iiFilePreview_hello_world)
endif()

set(_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/iiFilePreview")
configure_package_config_file(
    cmake/iiFilePreviewConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/iiFilePreviewConfig.cmake"
    INSTALL_DESTINATION "${_package_dir}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/iiFilePreviewConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(TARGETS iiFilePreview EXPORT iiFilePreviewTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES iiFilePreview.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT iiFilePreviewTargets NAMESPACE iiFilePreview:: DESTINATION "${_package_dir}")
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/iiFilePreviewConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/iiFilePreviewConfigVersion.cmake"
    DESTINATION "${_package_dir}"
)
install(FILES README.md DESTINATION "${CMAKE_INSTALL_DATADIR}/iiFilePreview")
