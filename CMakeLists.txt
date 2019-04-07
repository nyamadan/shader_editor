cmake_minimum_required(VERSION 3.1.0)
project(shader_editor VERSION 0.0.1)

set(MSVC_RUNTIME "dll")

set(APP_SOURCES
    src/main.cpp
    src/image.cpp
    src/platform.cpp
    src/app_log.cpp
    src/shader_utils.cpp
    src/shader_program.cpp
    src/file_utils.cpp
    src/webm_encoder.cpp
    src/mymkvwriter.cpp
    src/mp4muxer.c
)

set(GL3W_SOURCES
    gl3w/src/gl3w.c
)

set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_widgets.cpp
    imgui/examples/imgui_impl_glfw.cpp
    imgui/examples/imgui_impl_opengl3.cpp
    ImGuiColorTextEdit/TextEditor.cpp
)

#minimp4
include_directories(./minimp4)

#GLM
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
include_directories(./glm)

# Imgui
include_directories(./imgui)
include_directories(./ImGuiColorTextEdit)

# WebM
add_subdirectory(./libyuv)
include_directories(./libyuv/include)

add_subdirectory(./libwebm)
include_directories(./libwebm)

# add_subdirectory(./libvpx)
include_directories(./libvpx)

# stb
include_directories(./stb)

if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17")

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_EXE_LINKER_FLAGS "--embed-file ${PROJECT_SOURCE_DIR}/default@default --embed-file ${PROJECT_SOURCE_DIR}/assets@assets --no-heap-copy -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")

    # VPX
    set(LIB_VPX ${PROJECT_SOURCE_DIR}/libvpx.a)

    add_executable(shader_editor ${APP_SOURCES} ${IMGUI_SOURCES})
    add_dependencies(shader_editor webm yuv)
    target_link_libraries(shader_editor ${LIB_VPX} webm yuv)
elseif(MSVC)
    # C++17
    set (CMAKE_CXX_STANDARD 17)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /J")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /J")

    # OpenGL
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})

    # gl3w
    include_directories(./gl3w/include)

    # GLFW
    add_subdirectory(./glfw)
    include_directories(./glfw/include)

    # VPX
    set(LIB_VPX ${PROJECT_SOURCE_DIR}/vpxmd.lib)

    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES -D_SCL_SECURE_NO_WARNINGS)
    add_executable(shader_editor ${APP_SOURCES} ${IMGUI_SOURCES} ${GL3W_SOURCES})
    add_dependencies(shader_editor glfw webm yuv)
    target_link_libraries(shader_editor ${OPENGL_LIBRARIES} ${LIB_VPX} glfw webm yuv)
else()
    # C++17
    set (CMAKE_CXX_STANDARD 17)

    # OpenGL
    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})

    # gl3w
    include_directories(./gl3w/include)

    # GLFW
    add_subdirectory(./glfw)
    include_directories(./glfw/include)

    # VPX
    set(LIB_VPX ${PROJECT_SOURCE_DIR}/libvpx.a)

    add_executable(shader_editor ${APP_SOURCES} ${IMGUI_SOURCES} ${GL3W_SOURCES})
    target_link_libraries(shader_editor ${OPENGL_LIBRARIES} ${LIB_VPX} glfw webm yuv)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
