cmake_minimum_required(VERSION 3.10)

if(${CMAKE_VERSION} VERSION_LESS 3.14)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.14)
endif()

project(ToyRT2018 VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(TOYRT2018_USE_EXTERNAL_JSON OFF CACHE BOOL "Use an external JSON library")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(isMultiConfig)
	if(NOT "Profile" IN_LIST CMAKE_CONFIGURATION_TYPES)
		list(APPEND CMAKE_CONFIGURATION_TYPES Profile)
	endif()
else()
	set(allowableBuildTypes Debug Release RelWithDebInfo)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
	STRINGS "${allowableBuildTypes}")
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
	elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowableBuildTypes)
		message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
	endif()
endif()

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    set(GIT_SUBMODULE ON CACHE BOOL "Check submodules during build")
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

#GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "")

add_subdirectory("extern/glm")

#PCG
add_subdirectory("extern/pcg-cpp")

#TBB with CMake: https://github.com/wjakob/tbb
set(TBB_BUILD_SHARED           ON CACHE INTERNAL      "Build TBB shared library" )
set(TBB_BUILD_STATIC           ON CACHE INTERNAL       "Build TBB static library" )
set(TBB_BUILD_TBBMALLOC        ON CACHE INTERNAL      "Build TBB malloc library")
set(TBB_BUILD_TBBMALLOC_PROXY  OFF CACHE INTERNAL "Build TBB malloc proxy library" FORCE)
set(TBB_BUILD_TESTS            OFF CACHE INTERNAL "Build TBB tests and enable testing infrastructure" FORCE)
add_subdirectory(extern/tbbcmake)

#Embree, ref: https://github.com/embree/embree/blob/master/tutorials/find_embree/CMakeLists.txt
if (UNIX)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) #x64
        set(embree_DIR "/usr/lib64/cmake/embree-3.4.0/")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) #x86
        set(embree_DIR "/usr/lib32/cmake/embree-3.4.0/")
    endif()
endif(UNIX)

set(EMBREE_ISPC_SUPPORT OFF CACHE BOOL "Build Embree with support for ISPC applications.")
set(EMBREE_TUTORIALS OFF CACHE BOOL "Build Embree tutorials.")
#SET(EMBREE_TBB_ROOT "")
#SET(TBB_INCLUDE_DIR "extern/tbbcmake/include")
#SET(TBB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/extern/tbbcmake/libtbb_static.a")
#SET(TBB_LIBRARY_MALLOC "${CMAKE_CURRENT_BINARY_DIR}/extern/tbbcmake/libtbbmalloc_static.a")
FIND_PACKAGE(embree 3.0 REQUIRED)

#nlohmann_json
if(TOYRT2018_USE_EXTERNAL_JSON)
    find_package(nlohmann_json 3.2.0 REQUIRED)
else()
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(extern/nlohmann_json)
endif()

#AssImp
#set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "If the supplementary tools for Assimp are built in addition to the library.")
#set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "If the test suite for Assimp is built in addition to the library.")
#Disabling all importers by default, see CMakeLists output for importers supported and enable as needed
#set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE CACHE INTERNAL "Disable all importers by default.")
#set(ASSIMP_BUILD_OBJ_IMPORTER TRUE CACHE INTERNAL "Enable OBJ importer.")
#set(ASSIMP_BUILD_PLY_IMPORTER TRUE CACHE INTERNAL "Enable PLY importer.")
# https://github.com/assimp/assimp/issues/2179
#set(ASSIMP_BUILD_STEP_IMPORTER FALSE CACHE INTERNAL "Disable step builder that causes mingw debug build to fail.")
#if (POLICY CMP0048)
#    cmake_policy(SET CMP0048 NEW)
#endif (POLICY CMP0048)
#add_subdirectory(extern/assimp)

#Define the library
add_library(ToyRT2018_lib INTERFACE)

#https://foonathan.net/blog/2018/10/17/cmake-warnings.html Use SYSTEM to suppress warnings
target_include_directories(ToyRT2018_lib SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/extern/glm/")
target_include_directories(ToyRT2018_lib SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/extern/pcg-cpp/include")
target_include_directories(ToyRT2018_lib SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/extern/nlohmann_json/single_include")
#target_include_directories(ToyRT2018_lib SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/extern/assimp/include")
target_include_directories(ToyRT2018_lib SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/extern/tbbcmake/include")
target_include_directories(ToyRT2018_lib SYSTEM INTERFACE "${EMBREE_INCLUDE_DIRS}")
#set_target_properties(ToyRT2018_lib PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(ToyRT2018_lib INTERFACE include/)


add_executable(ToyRT2018 src/main.cpp)
target_link_libraries(ToyRT2018 PUBLIC ${EMBREE_LIBRARY})
target_link_libraries(ToyRT2018 PUBLIC nlohmann_json::nlohmann_json)
#target_link_libraries(ToyRT2018 PUBLIC assimp::assimp)
target_link_libraries(ToyRT2018 PUBLIC tbb_static)
target_link_libraries(ToyRT2018 PUBLIC ToyRT2018_lib)

if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU" )
    target_compile_options( ToyRT2018 PRIVATE -Wall -Wextra -Wunreachable-code -Wpedantic)
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    target_compile_options( ToyRT2018 PRIVATE -Wexit-time-destructors -Wglobal-constructors -Wmissing-noreturn)
endif()
if ( CMAKE_CXX_COMPILER_ID MATCHES "MSVC" )
    target_compile_options( ToyRT2018 PRIVATE /W4 /w44265 /w44061 /w44062 )
endif()

message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")