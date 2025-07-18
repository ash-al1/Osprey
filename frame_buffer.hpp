//===----------------------------------------------------------------------===//
//
// Library for converting a frame buffer to texture,
// for rendering in an ImGui viewport
//
// Reference: https://uysalaltas.github.io/2022/01/09/OpenGL_Imgui.html
//
//===----------------------------------------------------------------------===//

#ifndef FRAME_BUFFER_HPP
#define FRAME_BUFFER_HPP

class FrameBuffer
{
public:
    FrameBuffer(int width, int height);
    ~FrameBuffer();
    unsigned int getFrameTexture();
    void bind() const;
    void unbind() const;

private:
    unsigned int fbo;
    unsigned int texture;
    unsigned int rbo;
};

#endif