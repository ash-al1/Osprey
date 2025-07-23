//===----------------------------------------------------------------------===//
//
// Grid class for constructing, buffering, and drawing 3D grid with OpenGL
// 
//===----------------------------------------------------------------------===//

#ifndef GRID_HPP
#define GRID_HPP

// #include <glad/glad.h>

#include <cstddef> // for size_t

#include <glad/glad.h>

class Grid
{
public:
    /// constructor build to xy grid and the element indices
    /// and binding the OpenGL buffer objects
    ///
    /// z attribute index will be attribute + 1
    /// if uv = true, the uv attribute index will be attribidx + 2
    ///
    /// \param z                pointer to an array stroing the z-coordinates 
    ///                         2-dimensional, stored in row-major format
    ///                         (array must have a length of nRowsV * nColsV)
    ///     
    /// \param nRowsV           number of rows in the vertices array
    ///     
    /// \param nColsV           number of columns in the vertices array
    ///
    /// \param baseAttribIdx    the attribute index of the XY grid in the
    ///                         vertex shader,
    ///                         z attribute index will be attribute + 1,
    ///                         if uv = true, the uv attribute index 
    ///                         will be attribidx + 2
    ///
    /// \param uv               includes UV coordinates to the array, 
    ///                         defaulted to false
    /// 
    /// \param xR               the x-coordinate of the right edge, 
    ///                         defaulted to 1
    ///     
    /// \param xL               the x-coordinate of the left edge, 
    ///                         defaulted to -1
    ///     
    /// \param yT               the y-coordinate of the top edge, 
    ///                         defaulted to 1
    ///     
    /// \param yB               the y-coordinate of the bottom edge, 
    ///                         defaulted to -1
    ///
    /// \param xyUsage          XY buffer data usage (see glBufferData)
    ///                         defaulted to GL_STATIC_DRAW
    ///
    /// \param zUsage           Z buffer data usage (see glBufferData)
    ///                         defaulted to GL_DYNAMIC_DRAW
    /// 
    Grid(const float* z,
         const int nRowsV, 
         const int nColsV,
         const int baseAttribIdx,
         const bool uv = false,
         const float xR = 1.0f, const float xL = -1.0f,
         const float yT = 1.0f, const float yB = -1.0f,
         const GLenum xyUsage = GL_STATIC_DRAW,
         const GLenum zUsage =  GL_DYNAMIC_DRAW);


    ~Grid();


    /// OpenGL draw
    ///
    void draw();


    /// Substitude ALL the z data without reallocating the buffer
    ///
    /// \param newZ     pointer to an array stroing the z-coordinates 
    ///                 can be the same buffer as z in the constructor
    ///                 2-dimensional, stored in row-major format
    ///                 (array must have a length of nRowsV * nColsV)
    ///
    void zSubAllData(const float* newZ);


    /// Switch between log scaled grid and evenly spaced grid
    ///
    void gridSwitchLogScale();

    /// \return     whether the grid is in log scale
    ///
    bool getLogScale();

private:
    unsigned int VAO, xyVBO, zVBO, EBO;

    // storing inputs
    int nRowsV, nColsV;
    bool uv;
    float xR; float xL;
    float yT; float yB;

    int elementIndicesLen;

    bool logScale;
    int gridArrayLen;
    size_t gridArraySize;

    size_t zSize;
};

#endif