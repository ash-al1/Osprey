#pragma once

#include "grid.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include <memory>
#include <vector>

class FrameBuffer;

/**
 * Sexy 3D OpenGL Spectrogram Renderer
 */
class Spectro3D {
public:
	Spectro3D(int width=800, int height=600);
	~Spectro3D();

	void render();
	unsigned int getTextureID() const;
	
	int getWidth() const{ return width_; }
	int getHeight() const{ return height_; }
	bool isInitialized() const { return initialized_; }

	void handleMouseDrag(double x, double y, bool left_button, bool right_button);
    void handleMouseScroll(double yoffset);
    void resetView();

    void updateWaterfallData(const float* magnitude_data, int data_length);

private:
	int width_, height_;
    bool initialized_;

    std::unique_ptr<FrameBuffer> framebuffer_;
    bool initialize();
    void cleanup();

    Spectro3D(const Spectro3D&) = delete;
    Spectro3D& operator=(const Spectro3D&) = delete;

	std::unique_ptr<Grid> grid_;
    std::unique_ptr<Shader> shader_;
    std::unique_ptr<Camera> camera_;

	std::vector<float> grid_z_data_;
    int grid_rows_, grid_cols_;
    bool create3DGrid();

	bool mouse_dragging_;
    bool right_mouse_dragging_;
    double last_mouse_x_, last_mouse_y_;

};
