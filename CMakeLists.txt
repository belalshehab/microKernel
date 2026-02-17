cmake_minimum_required(VERSION 3.20)
project(MicroKernel)

set(CMAKE_CXX_STANDARD 20)

# Microkernel Architecture Components
add_executable(orchestrator orchestrator.cpp)
add_executable(hasher hasher.cpp)
add_executable(signer signer.cpp)