cmake_minimum_required(VERSION 3.16)
project(signals)
set(CMAKE_CXX_STANDARD 17)

# Find packages
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(PkgConfig REQUIRED)

# Core dependencies
pkg_check_modules(UHD REQUIRED uhd)

# Optional SDR libraries
pkg_check_modules(RTLSDR rtl-sdr)
pkg_check_modules(HACKRF hackrf)

# Try to find PFFFT using pkg-config first, then fallback to manual linking
pkg_check_modules(PFFFT pffft)
if(NOT PFFFT_FOUND)
    # Fallback: assume system installation in standard locations
    find_library(PFFFT_LIBRARY NAMES PFFFT pffft)
    find_path(PFFFT_INCLUDE_DIR NAMES pffft.h)
    
    if(PFFFT_LIBRARY AND PFFFT_INCLUDE_DIR)
        set(PFFFT_FOUND TRUE)
        set(PFFFT_LIBRARIES ${PFFFT_LIBRARY})
        set(PFFFT_INCLUDE_DIRS ${PFFFT_INCLUDE_DIR})
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/implot)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
include_directories(${UHD_INCLUDE_DIRS})

# Add PFFFT include directories if found
if(PFFFT_FOUND)
    include_directories(${PFFFT_INCLUDE_DIRS})
endif()

# Add include directories for optional SDR libraries
if(RTLSDR_FOUND)
    include_directories(${RTLSDR_INCLUDE_DIRS})
    add_definitions(-DHAS_RTLSDR)
endif()

if(HACKRF_FOUND)
    include_directories(${HACKRF_INCLUDE_DIRS})
    add_definitions(-DHAS_HACKRF)
endif()

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

# Core SDR HAL sources
set(CORE_HAL_SOURCES
    SDRFactory.cpp
    SimulationDevice.cpp
    USRPDevice.cpp
    FFTProcessor.cpp
    STFTSpectrogram.cpp
)

# Optional device sources
set(HAL_SOURCES ${CORE_HAL_SOURCES})
if(RTLSDR_FOUND)
    list(APPEND HAL_SOURCES RTLSDRDevice.cpp)
endif()

if(HACKRF_FOUND)
    list(APPEND HAL_SOURCES HackRFDevice.cpp)
endif()

# Application source files
set(APP_SOURCES
    main.cpp
    SignalGui.cpp
    UsrpController.cpp
    ${HAL_SOURCES}
)

# OPENGL
set(OPENGL_UTILS_SOURCES
    array2d.cpp
    camera.cpp
    frame_buffer.cpp
    grid.cpp
    shader.cpp
	glad/src/glad.c
	Spectro3D.cpp
)

# Create main executable
add_executable(${PROJECT_NAME}
    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
    ${APP_SOURCES}
	${OPENGL_UTILS_SOURCES}
)

# Link libraries
set(LINK_LIBRARIES
    glfw
    OpenGL::GL
    Boost::program_options
    ${UHD_LIBRARIES}
    pthread
    m
)

# Add PFFFT libraries if found
if(PFFFT_FOUND)
    list(APPEND LINK_LIBRARIES ${PFFFT_LIBRARIES})
endif()

# Add LibTorch libraries if found
if(LIBTORCH_FOUND)
    list(APPEND LINK_LIBRARIES ${TORCH_LIBRARIES})
    # Set the rpath for LibTorch (dynamic based on where it was found)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "${TORCH_INSTALL_PREFIX}/lib"
        BUILD_WITH_INSTALL_RPATH TRUE)
endif()

# Add optional SDR libraries
if(RTLSDR_FOUND)
    list(APPEND LINK_LIBRARIES ${RTLSDR_LIBRARIES})
endif()

if(HACKRF_FOUND)
    list(APPEND LINK_LIBRARIES ${HACKRF_LIBRARIES})
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES} glm::glm)

# Set C++17 and LibTorch compiler flags
if(LIBTORCH_FOUND)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_LIBTORCH)
    # LibTorch requires these flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
endif()

# Test programs
add_executable(test_circular_buffer TestCircularBuffer.cpp)
add_executable(test_usrp_controller TestUsrpController.cpp UsrpController.cpp)
target_link_libraries(test_usrp_controller ${UHD_LIBRARIES} pthread)

# HAL test program
add_executable(test_hal 
    TestHAL.cpp
    ${HAL_SOURCES}
    UsrpController.cpp
)
target_link_libraries(test_hal 
    ${UHD_LIBRARIES} 
	${PFFFT_LIBRARIES}
    pthread
	m
)

# Optional: RTL-SDR specific test
if(RTLSDR_FOUND)
    add_executable(test_rtlsdr TestRTLSDR.cpp RTLSDRDevice.cpp SDRFactory.cpp)
    target_link_libraries(test_rtlsdr ${RTLSDR_LIBRARIES} pthread)
endif()

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  UHD (USRP):    Found")
message(STATUS "  RTL-SDR:       ${RTLSDR_FOUND}")
message(STATUS "  HackRF:        ${HACKRF_FOUND}")
message(STATUS "  PFFFT:         ${PFFFT_FOUND}")
message(STATUS "  LibTorch:      ${LIBTORCH_FOUND}")
if(PFFFT_FOUND)
    message(STATUS "  PFFFT Include: ${PFFFT_INCLUDE_DIRS}")
    message(STATUS "  PFFFT Libs:    ${PFFFT_LIBRARIES}")
endif()
if(LIBTORCH_FOUND)
    message(STATUS "  LibTorch Path: ${TORCH_INSTALL_PREFIX}")
    message(STATUS "  LibTorch Libs: ${TORCH_LIBRARIES}")
endif()
message(STATUS "")
