if(DEFINED ENV{VCPKG_HOME})
    include("$ENV{VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake")
else()
    include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

set(VCPKG_INCLUDE_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/include") 
set(VCPKG_LIBRARY_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/lib") 
set(VCPKG_BIN_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/bin") 
set(VCPKG_DEBUG_BIN_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/debug/bin") 
set(VCPKG_DEBUG_LIBRARY_DIR "$ENV{VCPKG_HOME}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib") 
include_directories(${VCPKG_INCLUDE_DIR})

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(SFML COMPONENTS system network window graphics CONFIG REQUIRED)
find_package(muparser CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
  
file(GLOB_RECURSE SOURCES "src/*.cpp"  "include/*.h")
add_executable (Filigram "main.cpp" ${SOURCES})
add_executable (FiligramServer "server.cpp")

target_link_libraries(Filigram PRIVATE muparser::muparser)
target_link_libraries(Filigram PRIVATE unofficial::sqlite3::sqlite3)
target_link_libraries(Filigram PRIVATE sfml-system sfml-network sfml-graphics sfml-window)
target_link_libraries(Filigram PRIVATE imgui::imgui)
target_link_libraries(Filigram PRIVATE ImGui-SFML::ImGui-SFML)
target_link_libraries(Filigram PRIVATE implot::implot)
target_link_libraries(Filigram PRIVATE nlohmann_json::nlohmann_json)


target_link_libraries(FiligramServer PRIVATE unofficial::sqlite3::sqlite3)
target_link_libraries(FiligramServer PRIVATE sfml-system sfml-network)
target_link_libraries(FiligramServer PRIVATE nlohmann_json::nlohmann_json)
   
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Filigram PROPERTY CXX_STANDARD 20)
  set_property(TARGET FiligramServer PROPERTY CXX_STANDARD 20)
endif()

#Resources copy
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/Assets)
set(DESTINATION_DIR ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME})
file(COPY ${SOURCE_DIR} DESTINATION ${DESTINATION_DIR})
#Resources copy
