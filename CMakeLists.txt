cmake_minimum_required(VERSION 3.5.0)
project(pathtracer VERSION 0.1.0 LANGUAGES C)

find_package(Vulkan REQUIRED)
add_subdirectory(deps/stb_image_write)

add_executable(${PROJECT_NAME}
    src/main.c
    src/extensions.c
    src/utils.h
    src/utils.c
)

target_link_libraries(${PROJECT_NAME}
    Vulkan::Vulkan
    stb_image_write
)

set(SHADER_SOURCES
    shaders/pathtracer.comp
)

set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

foreach(SHADER_FILE ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv"
        COMMAND Vulkan::glslc ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE} -o ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling ${SHADER_FILE} to SPIR-V with glslc"
    )
    list(APPEND SPIRV_FILES "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
endforeach()

add_custom_target(shaders_target ALL DEPENDS ${SPIRV_FILES})

add_dependencies(${PROJECT_NAME} shaders_target)