cmake_minimum_required(VERSION 3.10)
project(Gizmos)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === External Dependencies ===

# GLFW
add_subdirectory("vendor/glfw")

# GLEW
set(ONLY_LIBS ON CACHE BOOL "Build only GLEW libraries, not executables")

add_subdirectory("vendor/glew")

# Assimp
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_DRACO_DECODER OFF)
set(ASSIMP_BUILD_ZLIB ON)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory("vendor/assimp")

# ImGui
set(IMGUI_SRC
    "vendor/imgui/imgui.cpp"
    "vendor/imgui/imgui_demo.cpp"
    "vendor/imgui/imgui_draw.cpp"
    "vendor/imgui/imgui_tables.cpp"
    "vendor/imgui/imgui_widgets.cpp"
    "vendor/imgui/backends/imgui_impl_glfw.cpp"
    "vendor/imgui/backends/imgui_impl_opengl3.cpp"
)

add_library(imgui STATIC ${IMGUI_SRC})

target_include_directories(imgui PUBLIC 
    "vendor/imgui"
    "vendor/imgui/backends"
)
target_link_libraries(imgui PUBLIC glfw)
set_target_properties(imgui PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/vendor/imgui"
)

# === Project Source Files ===
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(Gizmos ${SOURCES})

# === Shader Copy Post-Build Step ===
add_custom_command(
    TARGET Gizmos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Gizmos>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "${CMAKE_SOURCE_DIR}/src/shaders" 
            "$<TARGET_FILE_DIR:Gizmos>/shaders"
    COMMENT "Copying shader files to executable directory"
)

# === Include Paths ===
target_include_directories(Gizmos PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glew/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/include"
)

# === Linking ===
target_link_libraries(Gizmos PUBLIC
    glfw
    libglew_static
    imgui
    assimp
)

find_package(OpenGL REQUIRED)
target_link_libraries(Gizmos PRIVATE OpenGL::GL)
