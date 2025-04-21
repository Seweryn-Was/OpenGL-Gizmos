cmake_minimum_required(VERSION 3.10)
project(MyProject)

add_subdirectory("vendor/glfw")
add_subdirectory("vendor/glew")

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_SAMPLES OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_DRACO_DECODER OFF)
set(ASSIMP_BUILD_ZLIB ON)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("vendor/assimp")

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(Gizmos ${SOURCES} )

add_custom_command(
    TARGET Gizmos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/shaders" "shaders"
    COMMENT "Copying shader files to executable directory"
)
add_compile_definitions(SHADER_PATH="${CMAKE_SOURCE_DIR}/shaders")

set(ONLY_LIBS ON CACHE BOOL "Build only GLEW libraries, not executables")

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
target_include_directories(imgui PUBLIC "vendor/imgui" "vendor/imgui/backends")
target_link_libraries(imgui PUBLIC glfw)

set_target_properties(imgui PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/vendor/imgui"
)

target_include_directories(Gizmos
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glew/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/include"

)

if(WIN32)
    target_link_libraries(Gizmos PRIVATE opengl32)
endif()

target_link_libraries(Gizmos PUBLIC
    glfw
    libglew_static
    imgui
    assimp
)
