cmake_minimum_required(VERSION 3.30)
project(DirHTTP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(DirHTTP main.cpp)
target_include_directories(DirHTTP PRIVATE lib)

if(WIN32 AND MINGW)
    target_compile_definitions(DirHTTP PRIVATE
            WIN32_LEAN_AND_MEAN
            _WIN32_WINNT=0x0A00
            WINVER=0x0A00
    )

    target_link_libraries(DirHTTP PRIVATE ws2_32 shell32 iphlpapi)

    target_compile_options(DirHTTP PRIVATE
            -O2
            -flto
            -s
            -fno-rtti
    )

    target_link_options(DirHTTP PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
            -flto
            -Wl,--gc-sections
    )
endif()