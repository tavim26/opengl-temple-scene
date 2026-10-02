cmake_minimum_required(VERSION 3.16)

project(opengl-temple-scene
    VERSION 1.0
    DESCRIPTION "Interactive 3D temple scene rendered with OpenGL"
    LANGUAGES CXX
)

# ---------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type (Release if none is specified)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# compile_commands.json for IntelliSense in VS Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------
# Dependencies
#   sudo apt install build-essential cmake libglfw3-dev libglew-dev libglm-dev
# ---------------------------------------------------------------
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG QUIET)

if(NOT glm_FOUND)
    # GLM is header-only; fall back to the system include path
    find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
    add_library(glm::glm INTERFACE IMPORTED)
    set_target_properties(glm::glm PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}")
endif()

# ---------------------------------------------------------------
# Sources
# ---------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/Scene.cpp
    src/Lighting.cpp
    src/RainSystem.cpp
    src/Camera.cpp
    src/Mesh.cpp
    src/Model3D.cpp
    src/Shader.cpp
    src/SkyBox.cpp
    external/stb_image.cpp
    external/tiny_obj_loader.cpp
)

set(HEADERS
    src/Scene.hpp
    src/Lighting.hpp
    src/RainSystem.hpp
    src/GLUtils.hpp
    src/Camera.hpp
    src/Mesh.hpp
    src/Model3D.hpp
    src/Shader.hpp
    src/SkyBox.hpp
    external/stb_image.h
    external/tiny_obj_loader.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLM_ENABLE_EXPERIMENTAL   # required by <glm/gtx/transform.hpp> on newer GLM versions
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    glm::glm
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
    # Third-party single-header libraries: silence their warnings
    set_source_files_properties(external/stb_image.cpp external/tiny_obj_loader.cpp
        PROPERTIES COMPILE_OPTIONS "-w")
endif()

# ---------------------------------------------------------------
# Runtime
# The program loads shaders/, assets/, carriage/ and skybox/ through
# relative paths, so it must be run from the project root:
#   ./build/opengl-temple-scene
# ---------------------------------------------------------------