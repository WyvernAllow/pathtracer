cmake_minimum_required(VERSION 3.5.0)
project(pathtracer VERSION 0.1.0 LANGUAGES C)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.c
    src/extensions.c
)

target_link_libraries(${PROJECT_NAME}
    Vulkan::Vulkan
)