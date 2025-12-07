include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/vcpkg.cmake")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain")

if(EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    include("${CMAKE_TOOLCHAIN_FILE}")
else()
    message(FATAL_ERROR "vcpkg.cmake not found at: ${CMAKE_TOOLCHAIN_FILE}")
endif()


set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(spdlog CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(SFML COMPONENTS Graphics Window Audio Network System CONFIG REQUIRED)
find_package(muparser CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.*" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(FiligramCalc SHARED 
    "calc/Calc.cpp"
    "calc/Calc.h")
target_link_libraries(FiligramCalc PRIVATE muparser::muparser)
target_compile_definitions(FiligramCalc PRIVATE FILIGRAMCALC_EXPORTS)
target_include_directories(FiligramCalc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_options(FiligramCalc PRIVATE -Wl,--allow-multiple-definition)

file(GLOB_RECURSE Imgui_SFML_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../imgui-sfml/*.*")

add_executable(Filigram 
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    ${SOURCES}
    ${Imgui_SFML_SOURCES})
target_include_directories(Filigram PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(Filigram PRIVATE 
    unofficial::sqlite3::sqlite3
    Freetype::Freetype
    SFML::Graphics SFML::Network SFML::Window SFML::System
    imgui::imgui
    implot::implot
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    unofficial::argon2::libargon2
    unofficial-sodium::sodium
    FiligramCalc
    muparser::muparser
    OpenGL::GL)

file(GLOB_RECURSE SERVER_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/server/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/server/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/server/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/data/DatabaseManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/data/DatabaseManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/data/SQLModels.hpp")
add_executable(FiligramServer 
    "${CMAKE_CURRENT_SOURCE_DIR}/server.cpp"
    ${SERVER_SOURCES})
target_include_directories(FiligramServer PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/server/include")
target_link_libraries(FiligramServer PRIVATE 
    unofficial::sqlite3::sqlite3
    SFML::Graphics SFML::Network SFML::System
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    unofficial::argon2::libargon2
    unofficial-sodium::sodium
    FiligramCalc
    muparser::muparser)
    target_link_options(FiligramServer PRIVATE -Wl,--allow-multiple-definition)
    target_link_options(Filigram PRIVATE -Wl,--allow-multiple-definition)
# Копирование ресурсов в директорию сборки
set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
set(ASSETS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets")

if(EXISTS ${ASSETS_SOURCE_DIR})
    add_custom_command(TARGET Filigram POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSETS_SOURCE_DIR}
        ${ASSETS_DEST_DIR}
        COMMENT "Copying assets to build directory")
    
    add_custom_command(TARGET FiligramServer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSETS_SOURCE_DIR}
        ${ASSETS_DEST_DIR}
        COMMENT "Copying assets to build directory")
endif()
