cmake_minimum_required(VERSION 3.2.0)
project(shader_editor VERSION 0.0.1)

set(MSVC_RUNTIME "dll")

add_definitions(-DENABLE_HLSL)

set(APP_SOURCES
    ${PROJECT_SOURCE_DIR}/src/app.cpp
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/image.cpp
    ${PROJECT_SOURCE_DIR}/src/platform.cpp
    ${PROJECT_SOURCE_DIR}/src/app_log.cpp
    ${PROJECT_SOURCE_DIR}/src/shader_utils.cpp
    ${PROJECT_SOURCE_DIR}/src/shader_program.cpp
    ${PROJECT_SOURCE_DIR}/src/file_utils.cpp
    ${PROJECT_SOURCE_DIR}/src/webm_encoder.cpp
    ${PROJECT_SOURCE_DIR}/src/mymkvwriter.cpp
    ${PROJECT_SOURCE_DIR}/src/mp4muxer.c
    ${PROJECT_SOURCE_DIR}/src/h264_encoder.cpp
    ${PROJECT_SOURCE_DIR}/src/recording.cpp
    ${PROJECT_SOURCE_DIR}/src/buffers.cpp
    ${PROJECT_SOURCE_DIR}/src/shader_files.cpp
    ${PROJECT_SOURCE_DIR}/src/shader_compiler.cpp
)

set(GL3W_SOURCES
    ${PROJECT_SOURCE_DIR}/gl3w/src/gl3w.c
)

set(IMGUI_SOURCES
    ${PROJECT_SOURCE_DIR}/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/imgui/examples/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/imgui/examples/imgui_impl_opengl3.cpp
    ${PROJECT_SOURCE_DIR}/ImGuiColorTextEdit/TextEditor.cpp
)

#args
include_directories(${PROJECT_SOURCE_DIR}/args)

#minimp4
include_directories(${PROJECT_SOURCE_DIR}/minimp4)

#GLM
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
include_directories(${PROJECT_SOURCE_DIR}/glm)

# Imgui
include_directories(${PROJECT_SOURCE_DIR}/imgui)
include_directories(${PROJECT_SOURCE_DIR}/ImGuiColorTextEdit)

# WebM
add_subdirectory(${PROJECT_SOURCE_DIR}/libyuv)
include_directories(${PROJECT_SOURCE_DIR}/libyuv/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/libwebm)
include_directories(${PROJECT_SOURCE_DIR}/libwebm)
include_directories(${PROJECT_SOURCE_DIR}/libvpx)

# stb
include_directories(${PROJECT_SOURCE_DIR}/stb)

# glslang
add_subdirectory(${PROJECT_SOURCE_DIR}/glslang)

# SPIRV-Cross
add_subdirectory(${PROJECT_SOURCE_DIR}/SPIRV-Cross)

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17")

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_EXE_LINKER_FLAGS "--pre-js ${PROJECT_SOURCE_DIR}/settings.js --embed-file ${PROJECT_SOURCE_DIR}/assets@assets --no-heap-copy -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")

    # VPX
    set(LIB_VPX ${PROJECT_SOURCE_DIR}/libvpx.a)

    add_executable(shader_editor ${APP_SOURCES} ${IMGUI_SOURCES} ${PROJECT_SOURCE_DIR}/glslang/StandAlone/ResourceLimits.cpp)
    add_dependencies(shader_editor webm yuv glslang SPIRV spirv-cross-core spirv-cross-glsl)
    target_link_libraries(shader_editor ${LIB_VPX} webm yuv glslang SPIRV spirv-cross-core spirv-cross-glsl)
elseif(MSVC)
    # C++17
    set (CMAKE_CXX_STANDARD 17)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /J")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /J")

    # OpenGL
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})

    # gl3w
    execute_process(
        COMMAND python gl3w_gen.py
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/gl3w"
    )
    include_directories(${PROJECT_SOURCE_DIR}/gl3w/include)

    # GLFW
    add_subdirectory(${PROJECT_SOURCE_DIR}/glfw)
    include_directories(${PROJECT_SOURCE_DIR}/glfw/include)

    # VPX
    set(LIB_VPX ${PROJECT_SOURCE_DIR}/vpxmd.lib)

    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES -D_SCL_SECURE_NO_WARNINGS)
    add_executable(${PROJECT_NAME} ${APP_SOURCES} ${IMGUI_SOURCES} ${GL3W_SOURCES} ${PROJECT_SOURCE_DIR}/glslang/StandAlone/ResourceLimits.cpp)
    add_dependencies(${PROJECT_NAME} glfw webm yuv glslang SPIRV spirv-cross-core spirv-cross-glsl)
    target_link_libraries(${PROJECT_NAME}
        ${OPENGL_LIBRARIES}
        ${LIB_VPX}
        glfw webm yuv glslang SPIRV spirv-cross-core spirv-cross-glsl)
else()
    # C++17
    set (CMAKE_CXX_STANDARD 17)

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

    # OpenGL
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})

    # gl3w
    execute_process(
        COMMAND "python" "gl3w_gen.py"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/gl3w"
    )
    include_directories(${PROJECT_SOURCE_DIR}/gl3w/include)

    # GLFW
    add_subdirectory(${PROJECT_SOURCE_DIR}/glfw)
    include_directories(${PROJECT_SOURCE_DIR}/glfw/include)

    # VPX
    set(LIB_VPX ${PROJECT_SOURCE_DIR}/libvpx.a)

    add_executable(shader_editor ${APP_SOURCES} ${IMGUI_SOURCES} ${GL3W_SOURCES} ${PROJECT_SOURCE_DIR}/glslang/StandAlone/ResourceLimits.cpp)
    target_link_libraries(shader_editor ${OPENGL_LIBRARIES} ${LIB_VPX} glfw webm yuv glslang SPIRV spirv-cross-core spirv-cross-glsl)
endif()
