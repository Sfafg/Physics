cmake_minimum_required(VERSION 3.30.1)

project(Physics)

set(CMAKE_CXX_STANDARD 23)
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

set(TESTS_ROOT "${PROJECT_SOURCE_DIR}/tests")
set(SRC_ROOT "${PROJECT_SOURCE_DIR}/src")
set(RESOURCES_ROOT "${PROJECT_SOURCE_DIR}/resources")
set(RESOURCES_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources")
set(SHADER_COMPILER_DIR "C:/VulkanSDK/1.3.261.1/Bin/glslc.exe")
set(SHADER_ROOT "${SRC_ROOT}/shaders")
set(SHADER_OUTPUT_DIR "${RESOURCES_OUTPUT_DIR}/shaders")
set(CMAKE_EXE_LINKER_FLAGS "-static")

# set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra")
# set(CMAKE_CXX_FLAGS "-g -pg -no-pie")
set(SRC
    ${SRC_ROOT}/Renderer.cpp
    ${SRC_ROOT}/Components.cpp
    ${SRC_ROOT}/Constraints.cpp
    ${SRC_ROOT}/ColliderManager.cpp
    ${SRC_ROOT}/GPUDrivenRendererSystem.cpp
    ${SRC_ROOT}/DepthPyramid.cpp
    ${SRC_ROOT}/Mesh.cpp
    ${SRC_ROOT}/ObjLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/imgui_impl_vulkan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/imgui_demo.cpp
)

set(SHADERS
    ${SHADER_ROOT}/Renderer.comp
    ${SHADER_ROOT}/ClearInstructions.comp
    ${SHADER_ROOT}/Reduction.comp
    ${SHADER_ROOT}/shader.frag
    ${SHADER_ROOT}/shader.vert
    ${SHADER_ROOT}/passthrough.vert
)

set(RESOURCES
    ${RESOURCES_ROOT}/Box.obj
    ${RESOURCES_ROOT}/Kieliszek.obj
    ${RESOURCES_ROOT}/Parasol.obj
    ${RESOURCES_ROOT}/Parasolka.obj
    ${RESOURCES_ROOT}/Stolek.obj
    ${RESOURCES_ROOT}/Stolik.obj
    ${RESOURCES_ROOT}/Ziemia.obj
    ${RESOURCES_ROOT}/LibreBaskerville-Regular.ttf
)

file(MAKE_DIRECTORY ${TESTS_ROOT} ${SRC_ROOT} ${RESOURCES_ROOT} ${RESOURCES_OUTPUT_DIR} ${SHADER_ROOT} ${SHADER_OUTPUT_DIR})

# SHADERS
foreach(SHADER ${SHADERS})
    string(REPLACE "${SHADER_ROOT}/" "" SHADER_NAME "${SHADER}")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv"
        COMMAND "${SHADER_COMPILER_DIR}" "${SHADER}" -I"${CMAKE_CURRENT_SOURCE_DIR}/include" -o "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv"
        DEPENDS "${SHADER}"
        COMMENT "Compiling Shader: ${SHADER_NAME}.spv"
    )
    list(APPEND SHADER_DEPEND "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SHADER_DEPEND})

# RESOURCES
foreach(RESOURCE ${RESOURCES})
    string(REPLACE "${RESOURCES_ROOT}/" "" RESOURCE_NAME "${RESOURCE}")
    add_custom_command(
        OUTPUT "${RESOURCES_OUTPUT_DIR}/${RESOURCE_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy "${RESOURCE}" "${RESOURCES_OUTPUT_DIR}/${RESOURCE_NAME}"
        DEPENDS "${RESOURCE}"
        COMMENT "Copying resource: ${RESOURCE_NAME}"
    )
    list(APPEND RESOURCE_DEPEND "${RESOURCES_OUTPUT_DIR}/${RESOURCE_NAME}")
endforeach()

add_custom_target(Resources ALL DEPENDS ${RESOURCE_DEPEND})

# Physics
add_executable(Physics ${SRC} ${TESTS_ROOT}/Source.cpp)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(Physics PRIVATE VGraphics-d glfw3 vulkan-1)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(Physics PRIVATE VGraphics glfw3 vulkan-1)
endif()

target_precompile_headers(Physics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/VG/VG.h")

target_include_directories(Physics PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_directories(Physics PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/"
)
add_dependencies(Physics Shaders Resources)