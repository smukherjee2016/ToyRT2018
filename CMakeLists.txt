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

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
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

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/glm/CMakeLists.txt")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

#GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "")

add_subdirectory("extern/glm")
add_subdirectory("extern/pcg-cpp")

#OpenMP
find_package(OpenMP REQUIRED)

#Need to set embree Directory according to documentation
if(WIN32)
    set(embree_DIR "C:\\Program Files\\Intel\\Embree3 x64")
endif(WIN32)

find_package(embree 3 REQUIRED)

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
target_link_libraries(ToyRT2018 PUBLIC ToyRT2018_lib)

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU" )
    target_compile_options( ToyRT2018 PRIVATE -Wall -Wextra -Wunreachable-code -Wpedantic)
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options( ToyRT2018 PRIVATE -Wweak-vtables -Wexit-time-destructors -Wglobal-constructors -Wmissing-noreturn )
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options( ToyRT2018 PRIVATE /W4 /w44265 /w44061 /w44062 )
endif()