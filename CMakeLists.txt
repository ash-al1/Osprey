cmake_minimum_required(VERSION 3.16)
project(signals)
set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UHD REQUIRED uhd)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/implot)
include_directories(${UHD_INCLUDE_DIRS})

# ImGui source files
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

# ImPlot source files
set(IMPLOT_SOURCES
    implot/implot.cpp
    implot/implot_items.cpp
)

# Our application source files
set(APP_SOURCES
    main.cpp
    SignalGui.cpp
    UsrpController.cpp    # ADD THIS LINE
)

# Create executable
add_executable(${PROJECT_NAME}
    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
    ${APP_SOURCES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    glfw
    OpenGL::GL
    Boost::program_options
    ${UHD_LIBRARIES}
)
