cmake_minimum_required (VERSION 3.21)

project ("Filigram")

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_HOME})
        message(WARNING "VCPKG_HOME is set but CMAKE_TOOLCHAIN_FILE was not inherited. This may cause issues.")
    else()
        message(STATUS "Using default vcpkg resolution (Visual Studio built-in or system)")
    endif()
endif()

add_subdirectory ("Filigram")
