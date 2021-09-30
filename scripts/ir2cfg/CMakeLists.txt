cmake_minimum_required(VERSION 3.0)
project(llvm-tutor-hello-world)

#===============================================================================
# 1. LOAD LLVM CONFIGURATION
#===============================================================================
# Set this to a valid LLVM installation dir
set(CYY_LLVM_INSTALL_DIR "/root/Documents/llvm-project/build-10.0.1-release" CACHE PATH "LLVM installation directory")
set(CYY_LLVM_SRC_DIR "/root/Documents/llvm-project" CACHE PATH "LLVM source directory")

# Add the location of LLVMConfig.cmake to CMake search paths (so that
# find_package can locate it)
list(APPEND CMAKE_PREFIX_PATH "${CYY_LLVM_INSTALL_DIR}/lib/cmake/llvm/")
# message("CMAKE_SYSTEM_PREFIX_PATH: " ${CMAKE_SYSTEM_PREFIX_PATH})

message("CYY_LLVM_INSTALL_DIR: " ${CYY_LLVM_INSTALL_DIR})
message("CYY_LLVM_SRC_DIR: " ${CYY_LLVM_SRC_DIR})
message("CMAKE_PREFIX_PATH: " ${CMAKE_PREFIX_PATH})
find_package(LLVM REQUIRED CONFIG)
message("LLVM_LINK_LLVM_DYLIB: " ${LLVM_LINK_LLVM_DYLIB})
message("LLVM_DIR: " ${LLVM_DIR})

# HelloWorld includes headers from LLVM - update the include paths accordingly
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS}) # -isystem <directory>    Add directory to SYSTEM include search path
include_directories(${CYY_LLVM_SRC_DIR})
link_directories(${LLVM_LIBRARY_DIRS})

#===============================================================================
# 2. LLVM-TUTOR BUILD CONFIGURATION
#===============================================================================
# Use the same C++ standard as LLVM does
set(CMAKE_CXX_STANDARD 14 CACHE STRING "")

# LLVM is normally built without RTTI. Be consistent with that.
if(NOT LLVM_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

#===============================================================================
# 3. ADD THE TARGET
#===============================================================================
set(TARGET "main")
add_executable(${TARGET} main.cpp)
if (${LLVM_LINK_LLVM_DYLIB})
  set(llvm_libs LLVM)
else()
  llvm_map_components_to_libnames(llvm_libs bitwriter core ipo irreader instcombine instrumentation target linker analysis scalaropts support transformutils)
endif()

target_link_libraries(${TARGET} ${llvm_libs})

# set(CYY_SPDLOG_INSTALL_DIR "/root/downloads/git/spdlog/build" CACHE PATH "pdblog installation directory")
# list(APPEND CMAKE_PREFIX_PATH "${CYY_SPDLOG_INSTALL_DIR}")
# message("CYY_SPDLOG_INSTALL_DIR: " ${CYY_SPDLOG_INSTALL_DIR})
# find_package(spdlog REQUIRED)
# target_link_libraries(main PRIVATE spdlog::spdlog_header_only)
