cmake_minimum_required(VERSION 3.9)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.12)
endif()

project(ToyRT2018 VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
option(TOYRT2018_USE_EXTERNAL_JSON "Use an external JSON library" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
            "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/glm/CMakeLists.txt"
        OR NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/tinyobjloader/CMakeLists.txt"
        OR NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/pcg-cpp/CMakeLists.txt")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

#GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "")

add_subdirectory("extern/glm")

#PCG
add_subdirectory("extern/pcg-cpp")

#TinyObjLoader
add_subdirectory("extern/tinyobjloader")

#OpenMP
find_package(OpenMP REQUIRED)

#Embree, ref: https://github.com/embree/embree/blob/master/tutorials/find_embree/CMakeLists.txt
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) #x64
        set(embree_DIR "C:\\Program Files\\Intel\\Embree3 x64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) #x86
        set(embree_DIR "C:\\Program Files\\Intel\\Embree3 x86")
    endif()
endif(WIN32)
find_package(embree 3 REQUIRED)
include_directories(${EMBREE_INCLUDE_DIRS})
option(EMBREE_ISPC_SUPPORT "Build Embree with support for ISPC applications." OFF)

#nlohmann_json
if(TOYRT2018_USE_EXTERNAL_JSON)
    find_package(nlohmann_json 3.2.0 REQUIRED)
else()
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(extern/nlohmann_json)
endif()


add_library(ToyRT2018_lib INTERFACE)
target_sources( ToyRT2018_lib
        INTERFACE
        "${CMAKE_CURRENT_LIST_DIR}/include/"
        )
target_include_directories(ToyRT2018_lib INTERFACE "${CMAKE_CURRENT_LIST_DIR}/extern/glm/")
target_include_directories(ToyRT2018_lib INTERFACE "${CMAKE_CURRENT_LIST_DIR}/extern/pcg-cpp/include")
#set_target_properties(ToyRT2018_lib PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(ToyRT2018_lib INTERFACE include/)


add_executable(ToyRT2018 src/main.cpp)
if(OpenMP_CXX_FOUND)
    target_link_libraries(ToyRT2018 PUBLIC OpenMP::OpenMP_CXX)
endif()
target_link_libraries(ToyRT2018 PUBLIC ${EMBREE_LIBRARY})
target_link_libraries(ToyRT2018 PUBLIC ToyRT2018_lib)
target_link_libraries(ToyRT2018 PUBLIC tinyobjloader)
target_link_libraries(ToyRT2018 PUBLIC nlohmann_json::nlohmann_json)

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU" )
    target_compile_options( ToyRT2018 PRIVATE -Wall -Wextra -Wunreachable-code -Wpedantic)
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options( ToyRT2018 PRIVATE -Wweak-vtables -Wexit-time-destructors -Wglobal-constructors -Wmissing-noreturn )
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options( ToyRT2018 PRIVATE /W4 /w44265 /w44061 /w44062 )
endif()

message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")