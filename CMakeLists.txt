cmake_minimum_required(VERSION 3.21)
project(DSP)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_executable(DSP main.cpp WavSource.cpp WavSource.h WavHeader.h WavSink.cpp WavSink.h FIRFilter.cpp FIRFilter.h AGC.cpp AGC.h)

find_package(Volk REQUIRED)
if (${VOLK_FOUND})
    include_directories(${VOLK_INCLUDE_DIRS})
    target_link_libraries(DSP Volk::volk)
else ()
    message(FATAL_ERROR "Volk Required!")
endif ()