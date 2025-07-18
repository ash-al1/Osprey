#include "array2d.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

/// Linear interpolation for 2D coordinates
static float s_interp(float x , float xL, float yL, float xR, float yR)
{
    return yL + (x - xL) * (yR - yL) / (xR - xL);
}


int array2dIdx(const int i, const int j, const int nCols)
{
    return i * nCols + j;
}


void array2dMoveRowsUp(float* arr, 
                       const int nRows, 
                       const int nCols, 
                       const int n)
{
    assert(n < nRows);
    memmove(&arr[0], 
            &arr[n * nCols], 
            (nRows - n) * nCols * sizeof(float));
}


void array2dElementIndices(int* indicesArray,
                           const int nRowsQ,
                           const int nColsQ)
{
    for (int i = 0; i < nRowsQ; ++i)
    {
        for (int j = 0; j < nColsQ; ++j)
        {
            int baseIndex = 6 * array2dIdx(i , j, nColsQ);

            // repeated vertices
            int v0 = i * (nColsQ + 1) + (j + 1);
            int v2 = (i + 1) * (nColsQ + 1) + j;

            indicesArray[baseIndex] = v0;
            indicesArray[baseIndex + 1] = i * (nColsQ + 1) + j; // v1
            indicesArray[baseIndex + 2] = v2;
            indicesArray[baseIndex + 3] = v2;
            indicesArray[baseIndex + 4] = (i + 1) * (nColsQ + 1) + (j + 1);// v3
            indicesArray[baseIndex + 5] = v0;
        }   
    }
}


void array2dPatchIndices(int* indicesArray,
                         const int nRowsQ,
                         const int nColsQ,
                         array2dPatchOrder order)
{
    for (int i = 0; i < nRowsQ; ++i)
    {
        for (int j = 0; j < nColsQ; ++j)
        {
            int baseIndex = 4 * array2dIdx(i , j, nColsQ);

            switch (order)
            {
                case ARRAY2D_CCW:
                    indicesArray[baseIndex] = i * (nColsQ + 1) + (j + 1);
                    indicesArray[baseIndex + 1] = i * (nColsQ + 1) + j;
                    indicesArray[baseIndex + 2] = (i + 1) * (nColsQ + 1) + j;
                    indicesArray[baseIndex + 3] = (i + 1) * (nColsQ + 1) + (j + 1);
                    break;
                case ARRAY2D_GL_CCW:
                    indicesArray[baseIndex] = i * (nColsQ + 1) + j;
                    indicesArray[baseIndex + 1] = i * (nColsQ + 1) + (j + 1);
                    indicesArray[baseIndex + 2] = (i + 1) * (nColsQ + 1) + j;
                    indicesArray[baseIndex + 3] = (i + 1) * (nColsQ + 1) + (j + 1);
                    break;
                default: 
                    std::cout << "order not supported yet" << std::endl;
            }
        }   
    }    
}


void array2dGrid(float* gridArray,
                 const int nRowsV, 
                 const int nColsV,
                 const bool uv,
                 const float xR, const float xL,
                 const float yT, const float yB)
{
    for (int i = 0; i < nRowsV; ++i)
    {
        for (int j = 0; j < nColsV; ++j)
        {
            int baseIndex = (uv) ? 4 : 2;
            baseIndex *= array2dIdx(i , j, nColsV);

            // x
            // (xL, yL) is (0, xL) because we are mapping j to x
            gridArray[baseIndex] = (
                s_interp((float)(j),
                         0.0f, xL,
                         (float)(nColsV - 1), xR)
            );

            // y
            gridArray[baseIndex + 1] = (
                s_interp((float)(i),
                         0.0f, yT,
                         (float)(nRowsV - 1), yB)
            );

            if (uv)
            {
                // u
                gridArray[baseIndex + 2] = (
                    s_interp((float)(j),
                            0.0f, 0.0f,
                            (float)(nColsV - 1), 1.0f)
                );

                // v
                gridArray[baseIndex + 3] = (
                    s_interp((float)(i),
                            0.0f, 1.0f,
                            (float)(nRowsV - 1), 0.0f)
                );
            }
            else
            {
                // nothing, done already
            }
        }
    }
}


/// TODO:   Consider integrating this function into array2dGrid,
///         lots of code duplication
void array2dLogGrid(float* gridArray,
                    const int nRowsV, 
                    const int nColsV,
                    const bool uv,
                    const float xR, const float xL,
                    const float yT, const float yB)
{
    for (int i = 0; i < nRowsV; ++i)
    {
        for (int j = 0; j < nColsV; ++j)
        {
            int baseIndex = (uv) ? 4 : 2;
            baseIndex *= array2dIdx(i , j, nColsV);

            // x
            // (xL, yL) is (0, xL) because we are mapping j to x
            gridArray[baseIndex] = (
                s_interp(log10f((float)(1 + j)),
                         log10f(1.0f), xL,
                         log10f((float)(nColsV - 1)), xR)
            );

            // y
            gridArray[baseIndex + 1] = (
                s_interp((float)(i),
                         0.0f, yT,
                         (float)(nRowsV - 1), yB)
            );

            if (uv)
            {
                // u
                gridArray[baseIndex + 2] = (
                    s_interp((float)(j),
                            0.0f, 0.0f,
                            (float)(nColsV - 1), 1.0f)
                );

                // v
                gridArray[baseIndex + 3] = (
                    s_interp((float)(i),
                            0.0f, 1.0f,
                            (float)(nRowsV - 1), 0.0f)
                );
            }
            else
            {
                // nothing, done already
            }
        }
    }
}

/// TODO: y and j are probably wrong
/// TODO:   Consider splitting this function into x, y, u, v respectively
void array2dGridBatched(float* gridArray,
                        const int nRowsV, 
                        const int nColsV,
                        const bool uv,
                        const float xR, const float xL,
                        const float yT, const float yB)
{
    // this is an overcomplcated way of do batched array
    // but it should reduces the operation (at least for x and u)
    // from O(nRowsV * nColsV) to O(nRowsV + nColsV)
    // ---------------------------------------------------

    // x-elements
    // ----------
    // range(xL, xR) on the 1st row
    for (int j = 0; j < nColsV; ++j)
    {
        gridArray[j] = s_interp((float)(j), 
                                0.0f, xL,
                                (float)(nColsV - 1), xR);
    }

    // other rows are equal to the 1st since it is a rectangluar grid
    for (int i = 1; i < nRowsV; ++i) 
    {
        memcpy(&gridArray[i * nColsV], 
               &gridArray[0], 
               nColsV * sizeof(float));
    }

    // y-elements
    // ----------
    int yStartIdx = nRowsV * nColsV;

    for (int i = 0; i < nRowsV; ++i)
    {
        float yValue = s_interp((float)(i),
                                0.0f, yT,
                                (float)(nRowsV - 1), yB);

        for (int j = 0; j < nColsV; ++j)
        {
            gridArray[i * nColsV + yStartIdx + j] = yValue;
        }
    }

    // uv map (could use recurssion of the grid function)
    // --------------------------------------------------
    if (uv)
    {
        // u-elements
        // ----------
        int uStartIdx = nRowsV * nColsV * 2;

        #pragma omp parallel for
        for (int j = 0; j < nColsV; ++j)
        {
            gridArray[j + uStartIdx] = (
                s_interp((float)(j), 
                         0.0f, 0.0f,
                         (float)(nColsV - 1), 1.0f)
            );
        }

        // other rows are equal to the 1st since it is a rectangluar grid
        for (int i = 1; i < nRowsV; ++i) 
        {
            memcpy(&gridArray[i * nColsV + uStartIdx], 
                   &gridArray[uStartIdx], 
                   nColsV * sizeof(float));
        }

        // v-elements
        // ----------
        int vStartIdx = nRowsV * nColsV * 3;

        for (int i = 0; i < nRowsV; ++i)
        {
            float vValue = s_interp((float)(i),
                                    0.0f, 1.0f,
                                    (float)(nRowsV - 1), 0.0f);
            
            for (int j = 0; j < nColsV; ++j)
            {
                gridArray[i * nColsV + vStartIdx + j] = vValue;
            }
        }
    }
}
