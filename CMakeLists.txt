cmake_minimum_required(VERSION 3.5)
project(sandbox_game VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE DEFINITIONS src/*.c)

find_package(Vulkan REQUIRED)

find_package(glfw3 REQUIRED)

add_executable(${PROJECT_NAME} ${DEFINITIONS})

# https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
set(CMAKE_DEBUG_FLAGS "-Wall -fsanitize=address -g -fno-omit-frame-pointer")
set(CMAKE_OPTIMIZE_SPEED_FLAGS "-O3")
set(CMAKE_OPTIMIZE_BINARY_SIZE_FLAGS "-Os -s -fno-ident -fno-asynchronous-unwind-tables")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_DEBUG_FLAGS}")


set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 23)

target_include_directories(${PROJECT_NAME}
        PUBLIC vendor/stb
        PUBLIC src)

target_link_directories(${PROJECT_NAME}
        PRIVATE vendor/stb)

target_link_libraries(${PROJECT_NAME}
        glfw
        vulkan
        m
        pthread)