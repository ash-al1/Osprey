#include "Spectro3D.h"
#include "frame_buffer.hpp"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>

Spectro3D::Spectro3D(int width, int height, int time_bins, int freq_bins)
    : width_(width)
    , height_(height)
    , initialized_(false)
	, grid_rows_(time_bins)
	, grid_cols_(freq_bins)
	, mouse_dragging_(false)
    , right_mouse_dragging_(false)
    , last_mouse_x_(0.0)
    , last_mouse_y_(0.0) {
    initialized_ = initialize();
}

Spectro3D::~Spectro3D() {
    cleanup();
}

bool Spectro3D::initialize() {
    try {
        framebuffer_ = std::make_unique<FrameBuffer>(width_, height_);
        if (!framebuffer_) {
            std::cerr << "Failed to create framebuffer" << std::endl;
            return false;
        }
        
        if (!create3DGrid()) {
            std::cerr << "Failed to create 3D grid" << std::endl;
            return false;
        }
        
        std::cout << "Spectro3D initialized successfully (" << width_ << "x" << height_ << ")" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception during Spectro3D initialization: " << e.what() << std::endl;
        return false;
    }
}

void Spectro3D::cleanup() {
    framebuffer_.reset();
    grid_.reset();
    shader_.reset();
    camera_.reset();
}

void Spectro3D::render() {
    if (!initialized_ || !framebuffer_ || !grid_ || !shader_ || !camera_) {
        static bool error_logged = false;
        if (!error_logged) {
            std::cerr << "Spectro3D::render() called with uninitialized components:" << std::endl;
            std::cerr << "  initialized_: " << initialized_ << std::endl;
            std::cerr << "  framebuffer_: " << (framebuffer_ ? "OK" : "NULL") << std::endl;
            std::cerr << "  grid_: " << (grid_ ? "OK" : "NULL") << std::endl;
            std::cerr << "  shader_: " << (shader_ ? "OK" : "NULL") << std::endl;
            std::cerr << "  camera_: " << (camera_ ? "OK" : "NULL") << std::endl;
            error_logged = true;
        }
        return;
    }

    // Store previous viewport to restore later
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    framebuffer_->bind();
    glViewport(0, 0, width_, height_);

    // Clear with dark background
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing for proper 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    shader_->use();

    // Get transform matrix from camera
    glm::mat4 transform = camera_->getPVMMat();
    shader_->setMat4("rotationMat", transform);

    // Set colormap colors (teal to yellow like spectrolysis)
    shader_->setVec3("rgbColormap0", 0.906f, 1.000f, 0.529f);  // Light yellow
    shader_->setVec3("rgbColormap1", 0.000f, 0.502f, 0.502f);  // Teal

    // Draw the grid
    grid_->draw();

    framebuffer_->unbind();
    
    // Restore previous viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

unsigned int Spectro3D::getTextureID() const {
    if (framebuffer_) {
        return framebuffer_->getFrameTexture();
    }
    return 0;
}

bool Spectro3D::create3DGrid() {
    try {
        // Initialize grid Z-data with zeros
        grid_z_data_.resize(grid_rows_ * grid_cols_, 0.0f);

        shader_ = std::make_unique<Shader>("waterfall.vs", "waterfall.fs");
        camera_ = std::make_unique<Camera>();

        // Calculate proper aspect ratio to prevent stretching
        float aspect_ratio = static_cast<float>(grid_rows_) / static_cast<float>(grid_cols_);

        // Adjust coordinates to maintain proper aspect ratio
        float x_range = 1.0f;                    // Full width
        float y_range = x_range * aspect_ratio;  // Scaled height

        grid_ = std::make_unique<Grid>(
            grid_z_data_.data(),
            grid_rows_,
            grid_cols_,
            0,
            false,
            x_range, -x_range,    // xR, xL: -1 to +1 (frequency)
            y_range, -y_range     // yT, yB: scaled by aspect ratio (time)
        );

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating 3D grid: " << e.what() << std::endl;
        return false;
    }
}

void Spectro3D::handleMouseDrag(double x, double y, bool left_button, bool right_button) {
    if (camera_) {
        camera_->drag2Rotate(x, y, left_button);
        camera_->rightDrag2Move(x, y, right_button);
    }
}

void Spectro3D::handleMouseScroll(double yoffset) {
    if (camera_) {
        camera_->scroll2Zoom(yoffset);
    }
}

void Spectro3D::resetView() {
    if (camera_) {
        camera_->returnButton();
    }
}

void Spectro3D::updateWaterfallData(const float* magnitude_data, int data_length) {
    if (!grid_ || data_length != grid_cols_) {
        std::cerr << "Data length mismatch: expected " << grid_cols_ 
                  << ", got " << data_length << std::endl;
        return;
    }

    // Bounds check
    if (grid_z_data_.size() != static_cast<size_t>(grid_rows_ * grid_cols_)) {
        std::cerr << "Grid data size mismatch!" << std::endl;
        return;
    }

    // Debug: Check input data range occasionally
    static int debug_counter = 0;
    if (debug_counter++ % 100 == 0) {
        float min_val = magnitude_data[0], max_val = magnitude_data[0];
        for (int i = 0; i < data_length; ++i) {
            min_val = std::min(min_val, magnitude_data[i]);
            max_val = std::max(max_val, magnitude_data[i]);
        }
        std::cout << "Magnitude data range: [" << min_val << ", " << max_val << "]" << std::endl;
    }

    // More efficient: use memmove to shift all rows at once
    // This moves (grid_rows_-1) * grid_cols_ floats
    size_t bytes_to_move = (grid_rows_ - 1) * grid_cols_ * sizeof(float);
    std::memmove(grid_z_data_.data(), 
                 grid_z_data_.data() + grid_cols_, 
                 bytes_to_move);

    // Add new data to the last row
    int last_row_start = (grid_rows_ - 1) * grid_cols_;
    for (int col = 0; col < grid_cols_; ++col) {
        float value = magnitude_data[col];
        
        // Since magnitude data is already scaled 0-1 from FFT processor,
        // apply a simple gamma correction for better visual contrast
        value = std::pow(std::max(0.0f, std::min(1.0f, value)), 0.7f);
        
        grid_z_data_[last_row_start + col] = value;
    }

    grid_->zSubAllData(grid_z_data_.data());
}
