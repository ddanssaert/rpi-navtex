cmake_minimum_required(VERSION 3.16)
project(navtex_sdr_dsp CXX)
set(CMAKE_CXX_STANDARD 17)

find_library(SDRPLAY_LIB sdrplay_api HINTS /usr/local/lib REQUIRED)

add_executable(sdr-dsp
    src/main.cpp
    src/fir1.cpp
    src/fir2.cpp
    src/fir3.cpp
    src/decoder.cpp
    src/nav_b_sm.cpp
    src/message_publisher.cpp
)

target_include_directories(sdr-dsp PRIVATE src /usr/local/include third_party)
target_link_libraries(sdr-dsp PRIVATE ${SDRPLAY_LIB} pthread)
