cmake_minimum_required(VERSION 3.16)
project(Maze3DGenerator LANGUAGES CXX)

option(BUILD_TESTS "Build test executables" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable Qt's automatic MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

# Platform-specific definitions
if (WIN32)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Sources
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MazeGLWidget.cpp
    src/MazeGenerator.cpp
    src/MazeController.cpp
    src/MazeSolver.cpp
    src/MazeIO.cpp
)
set(HEADERS
    include/MainWindow.h
    include/MazeGLWidget.h
    include/MazeTypes.h
    include/MazeGrid.h
    include/MazeGenerator.h
    include/MazeController.h
)

add_executable(Maze3DGenerator ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(Maze3DGenerator PRIVATE include)

# Link Qt
target_link_libraries(Maze3DGenerator PRIVATE Qt6::Widgets Qt6::OpenGLWidgets)

# High DPI support on Windows/macOS
if (APPLE)
  target_compile_definitions(Maze3DGenerator PRIVATE QT_MACEXTRA)
endif()

# Install rules (optional)
install(TARGETS Maze3DGenerator RUNTIME DESTINATION bin)




if(BUILD_TESTS)
    add_executable(maze_tests tests/test_maze.cpp src/MazeGenerator.cpp
        src/MazeController.cpp
        src/MazeSolver.cpp
        src/MazeIO.cpp)
    target_include_directories(maze_tests PRIVATE include)
    target_link_libraries(maze_tests PRIVATE Qt6::Core Qt6::Gui)

    add_executable(maze_persistence_tests tests/test_persistence.cpp src/MazeIO.cpp src/MazeGenerator.cpp)
    target_include_directories(maze_persistence_tests PRIVATE include)
    target_link_libraries(maze_persistence_tests PRIVATE Qt6::Core Qt6::Gui)
endif()
