cmake_minimum_required(VERSION 3.15)
project(StrongSynthesizer VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# --- AUTOMATED DEPENDENCY DOWNLOAD (MINIAUDIO) ---
# Define the target directory and path for the single-header library
set(MINIAUDIO_DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/bridge")
set(MINIAUDIO_HEADER_PATH "${MINIAUDIO_DOWNLOAD_DIR}/miniaudio.h")

# Check if miniaudio.h is already present, if not, download it natively via CMake
if(NOT EXISTS "${MINIAUDIO_HEADER_PATH}")
    message(STATUS "miniaudio.h not found local. Fetching from official repository...")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h"
        "${MINIAUDIO_HEADER_PATH}"
        SHOW_PROGRESS
        STATUS download_status
    )
    
    # Validate the network download operation status rules
    list(GET download_status 0 error_code)
    if(NOT error_code EQUAL 0)
        list(GET download_status 1 error_message)
        message(FATAL_ERROR "Failed to download miniaudio.h: ${error_message}")
    endif()
    message(STATUS "miniaudio.h downloaded successfully to src/bridge/.")
endif()
# --------------------------------------------------

# Configure explicit miniaudio backend link dependencies for platform audio tracking
if(LINUX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT} m dl asound)
elseif(APPLE)
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIOUNIT_LIB AudioUnit)
    find_library(COCOA_LIB Cocoa)
    set(SYSTEM_LIBS ${COREAUDIO_LIB} ${AUDIOUNIT_LIB} ${COCOA_LIB} dl)
else()
    set(SYSTEM_LIBS "")
endif()

# Dynamically gather all source files to prevent manual entry mismatches
file(GLOB CORE_SOURCES "src/core/source/*.cpp")

# Isolate main.cpp out of the core list to avoid multiple 'main' symbol collisions
set(LIBRARY_SOURCES ${CORE_SOURCES})
list(FILTER LIBRARY_SOURCES EXCLUDE REGEX "src/core/source/main.cpp")

# Construct the central synthesis static object dependency
add_library(synth_core STATIC ${LIBRARY_SOURCES})
target_compile_definitions(synth_core PRIVATE MA_NO_VERSIONING)
target_include_directories(synth_core PUBLIC 
    src/core/include 
    src/bridge
)
target_link_libraries(synth_core PRIVATE ${SYSTEM_LIBS})

# Build the native diagnostics testing utility CLI binary
add_executable(synth_cli src/core/source/main.cpp)
target_include_directories(synth_cli PRIVATE 
    src/core/include 
    src/bridge
)
target_link_libraries(synth_cli PRIVATE synth_core ${SYSTEM_LIBS})

# Build the raw shared library object module needed by your python ctypes test setup
add_library(synth_bridge SHARED 
    src/bridge/bridge_bindings.cpp
)
target_include_directories(synth_bridge PRIVATE 
    src/core/include 
    src/bridge
)
target_link_libraries(synth_bridge PRIVATE synth_core ${SYSTEM_LIBS})

# Standardize output naming conventions across Linux, Windows, and macOS targets
set_target_properties(synth_bridge PROPERTIES 
    SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
    OUTPUT_NAME "synth_bridge"
)
