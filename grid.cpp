#include "grid.hpp"

#include <vector>

#include "array2d.hpp"

Grid::Grid(const float* z,
           const int nRowsV, 
           const int nColsV,
           const int baseAttribIdx,
           const bool uv,
           const float xR, const float xL,
           const float yT, const float yB,
           const GLenum xyUsage,
           const GLenum zUsage)
    :   nRowsV(nRowsV), nColsV(nColsV),
        uv(uv),
        xR(xR), xL(xL),
        yT(yT), yB(yB),
        logScale(true) // defaulted to log scale, hardcoded
{
    // generate xy grid and element indices
    // ------------------------------------
    // xy-vertices in gridArray
    // use vector size we don't need it after binding GL buffer
    this->gridArrayLen = uv ? nRowsV * nColsV * 4 : nRowsV * nColsV * 2;

    std::vector<float> gridArray(gridArrayLen);

    // defaulted to log scale
    array2dLogGrid(gridArray.data(), 
                   nRowsV, nColsV, 
                   uv, 
                   xR, xL, 
                   yT, yB);
    
    this->gridArraySize = gridArrayLen * sizeof(float);

    // z-vertices
    // ----------
    this->zSize = nRowsV * nColsV * sizeof(float);

    // element indices
    // ---------------
    this->elementIndicesLen = (nRowsV - 1) * (nColsV - 1) * 6;

    std::vector<int> elementIndicesArray(elementIndicesLen);

    array2dElementIndices(elementIndicesArray.data(), nRowsV - 1, nColsV - 1);
    
    size_t elementIndicesSize = elementIndicesLen * sizeof(int);

    // create and bind the OpenGL buffer objects
    // -----------------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &xyVBO);
    glGenBuffers(1, &zVBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    GLsizei stride; 
    uv? stride = 4 * sizeof(float) : stride = 2 * sizeof(float);

    // xy-coordinates buffer
    glBindBuffer(GL_ARRAY_BUFFER, xyVBO);
    glBufferData(GL_ARRAY_BUFFER, gridArraySize, &gridArray[0], xyUsage); 
    glVertexAttribPointer(baseAttribIdx, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(baseAttribIdx);

    if (uv)
    {
        glVertexAttribPointer(baseAttribIdx + 2, 2, GL_FLOAT, GL_FALSE, stride, 
                              (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(baseAttribIdx + 2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind xy buffer

    // z-coordinates buffer
    glBindBuffer(GL_ARRAY_BUFFER, zVBO);
    glBufferData(GL_ARRAY_BUFFER, zSize, &z[0], zUsage);
    glVertexAttribPointer(baseAttribIdx + 1, 1, GL_FLOAT, GL_FALSE, sizeof(float), 
                          (void*)0);
    glEnableVertexAttribArray(baseAttribIdx + 1);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind z buffer

    // element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementIndicesSize, &elementIndicesArray[0], xyUsage);

    // unbind VAO
    glBindVertexArray(0);
}


Grid::~Grid()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &xyVBO);
    glDeleteBuffers(1, &zVBO);
    glDeleteBuffers(1, &EBO);
}


void Grid::draw()
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, elementIndicesLen, GL_UNSIGNED_INT, 0);
}


void Grid::zSubAllData(const float* newZ)
{
    // bind zVBO to modify it
    glBindBuffer(GL_ARRAY_BUFFER, zVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, zSize, &newZ[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind buffer
}


void Grid::gridSwitchLogScale()
{
    std::vector<float> gridArray(gridArrayLen);

    if (logScale)
    {
        array2dGrid(gridArray.data(), 
                    nRowsV, nColsV, 
                    uv, 
                    xR, xL, 
                    yT, yB);
    }
    else
    {
        array2dLogGrid(gridArray.data(), 
                       nRowsV, nColsV, 
                       uv, 
                       xR, xL, 
                       yT, yB);
    }

    // bind xyVBO to modify it
    glBindBuffer(GL_ARRAY_BUFFER, xyVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gridArraySize, &gridArray[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind buffer

    // switch the state
    this->logScale = !logScale;
}


bool Grid::getLogScale()
{
    return this->logScale;
}