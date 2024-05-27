cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME course)
set(CMAKE_CXX_STANDARD 17)

set(GLFW_DEPS X11 Xrandr Xinerama Xcursor GL dl pthread)
# set(GLFW_DEPS X11 Xrandr Xinerama Xcursor -lglfw3 -lGLU -lGL)

set(WINDOW_NAME "Hello, OpenGL!")
set(WINDOW_WIDTH 640)
set(WINDOW_HEIGHT 480)

project(${PROJECT_NAME})
add_executable(${PROJECT_NAME} 
    # src/main.cpp
    # src/RotatingCube.cpp
    # src/Object-2.cpp
    # src/ImGui.cpp
    # src/Texture.cpp
    # src/ColoredTriangle.cpp
    src/drawSinglePoint.cpp
    # src/basicWindow.cpp
)

include(Dependency.cmake)

# 우리 프로젝트에 include / lib 관련 옵션 추가
target_include_directories(${PROJECT_NAME} PUBLIC ${DEP_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PUBLIC ${DEP_LIB_DIR})
# target_link_libraries(${PROJECT_NAME} PUBLIC ${DEP_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${DEP_LIBS} ${GLFW_DEPS} ${GLEW_STATIC_LIBRARY})

target_compile_definitions(${PROJECT_NAME} PUBLIC
    WINDOW_NAME="${WINDOW_NAME}"
    WINDOW_WIDTH=${WINDOW_WIDTH}
    WINDOW_HEIGHT=${WINDOW_HEIGHT}
    )

# Dependency들이 먼저 build 될 수 있게 관계 설정
add_dependencies(${PROJECT_NAME} ${DEP_LIST})
