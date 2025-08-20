#version 330 core

layout (location = 0) in vec2 aPosXY;
layout (location = 1) in float aPosZ;

uniform mat4 rotationMat;
uniform vec3 rgbColormap0;
uniform vec3 rgbColormap1;

out float height;
out vec3 rgb_colormap0;
out vec3 rgb_colormap1;

void main()
{
    float zScaling = 0.6;
    height = clamp(aPosZ / zScaling, 0.0, 1.0);

    // colormap base colors (can be overridden by uniforms)
    rgb_colormap0 = rgbColormap0;
    rgb_colormap1 = rgbColormap1;
    
    // Coordinate system:
    // X: -1 to 1 (frequency: low to high)
    // Y: 1 to -1 (time: old to new, flows from top to bottom)
    // Z: 0 to zScaling (amplitude: low to high peaks)
    gl_Position = rotationMat * vec4(aPosXY.x, aPosXY.y, aPosZ * zScaling, 1.0);
}
