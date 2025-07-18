//===----------------------------------------------------------------------===//
//
// Library for 2D array manipulation, stored in row majored order.
// 
//===----------------------------------------------------------------------===//
// Variable Conventions:
//  nRows:       number of rows for an arbitrary 2D array
//  nCols:       number of columns for an arbitrary 2D array 
//  nRowsQ:      number of rows in the quads array
//  nColsQ:      number of columns in the quads array
//  nRowsV:      number of rows in the vertices array
//  nColsV:      number of columns in the vertices array

#ifndef ARRAY2D_HPP
#define ARRAY2D_HPP

/// For patchIndices
///
typedef enum {
    ARRAY2D_CCW,      /// counter-clockwise order
    ARRAY2D_GL_CCW    /// OpenGL tessellation "ccw" order (not actual ccw)
} array2dPatchOrder;


/// Index of a flattened 2D array in row-major order
///
/// \param i        the row index of the 2D array, starts from i = 0
///
/// \param j        the column index of the 2D array, starts from j = 0
///
/// \param nCols    the number of columns of the 2D array
///
/// \return         index number, zero based numbering
///
int array2dIdx(const int i, const int j, const int nCols);


/// Move every row on a flattened 2D array n row(s) up, 
/// with the last n rows is untouched
///
/// (i.e. row 0 becomes row n, row 1 becomes row n + 1, etc.)
///
/// \param arr      pointer for the float array
///
/// \param nRows    number of rows
///
/// \param nCols    number of columns
///
/// \param n        number of rows to move
///
void array2dMoveRowsUp(float* arr, 
                       const int nRows, 
                       const int nCols, 
                       const int n);


/// Indices for converting a quadrilateral grid into triangular elements
///
/// Example: 1x1 quad
///    0---1
///    | / |
///    2---3
/// indices for each quad is organized as such = {1, 0, 2,  2, 3, 1}
///
/// \param indicesArray     pointer to an array storing the element indices
///                         2-dimensional, stored in row-major format
///                         (array must have a length of nRowsQ * nColsQ * 6)
///
/// \param nRowsQ           number of rows in the quads array
///
/// \param nColsQ           number of columns in the quads array
///
void array2dElementIndices(int* indicesArray,
                           const int nRowsQ,
                           const int nColsQ);


/// Indices for converting a quadrilateral grid into quadrilateral patches
///
/// Example: 1x1 quad
///    0---1
///    | / |
///    2---3
/// indices for each quad is organized as such
/// order = PATCH_CCW:      {1, 0, 2, 3} (counter-clockwise)
/// order = PATCH_GL_CCW:   {0, 1, 2, 3} (NOT counter-clockwise nor clockwise,
///                                      but OpenGL tessellation's wrong ccw!)
///
/// \param indicesArray     pointer to an array storing the patch indices
///                         2-dimensional, stored in row-major format
///                         (array must have a length of nRowsQ * nColsQ * 4)
///
/// \param nRowsQ           number of rows in the quads array
///     
/// \param nColsQ           number of columns in the quads array
///     
/// \param order            ordering, read documentation for more detail
///
void array2dPatchIndices(int* indicesArray,
                         const int nRowsQ,
                         const int nColsQ,
                         array2dPatchOrder order);


/// Creating a rectanglar evenly spaced xy-grid coordinates array, without
/// the z-coordinates
///
/// Stored in interleaved format [x0, y0, u0, v1, x1, y1, u1, v1, ........]
///
/// \param gridArray    pointer to an array storing the grid indices
///                     2-dimensional, stored in row-major format
///                     (must have a length of nRowsV * nColsV * 4 if uv = true
///                      or nRowsV * nColsV * 2 if uv = false)
///
/// \param nRowsV       number of rows in the vertices array
/// 
/// \param nColsV       number of columns in the vertices array
/// 
/// \param xR           the x-coordinate of the right edge, defaulted to 1
///     
/// \param xL           the x-coordinate of the left edge, defaulted to -1
///     
/// \param yT           the y-coordinate of the top edge, defaulted to 1
///     
/// \param yB           the y-coordinate of the bottom edge, defaulted to -1
/// 
/// \param uv           includes UV coordinates to the array, defaulted to false
///
void array2dGrid(float* gridArray,
                 const int nRowsV, 
                 const int nColsV,
                 const bool uv = false,
                 const float xR = 1.0f, const float xL = -1.0f,
                 const float yT = 1.0f, const float yB = -1.0f);


/// Creating a rectanglar log10 spaced xy-grid coordinates array, without
/// the z-coordinates
///
/// Stored in interleaved format [x0, y0, u0, v1, x1, y1, u1, v1, ........]
///
/// \param gridArray    pointer to an array storing the grid indices
///                     2-dimensional, stored in row-major format
///                     (must have a length of nRowsV * nColsV * 4 if uv = true
///                      or nRowsV * nColsV * 2 if uv = false)
///
/// \param nRowsV       number of rows in the vertices array
/// 
/// \param nColsV       number of columns in the vertices array
/// 
/// \param xR           the x-coordinate of the right edge, defaulted to 1
/// 
/// \param xL           the x-coordinate of the left edge, defaulted to -1
///     
/// \param yT           the y-coordinate of the top edge, defaulted to 1
///     
/// \param yB           the y-coordinate of the bottom edge, defaulted to -1
/// 
/// \param uv           includes UV coordinates to the array, defaulted to false
///
void array2dLogGrid(float* gridArray,
                    const int nRowsV, 
                    const int nColsV,
                    const bool uv = false,
                    const float xR = 1.0f, const float xL = -1.0f,
                    const float yT = 1.0f, const float yB = -1.0f);


/// Creating a rectanglar evenly spaced xy-grid coordinates array, without
/// the z-coordinates, arranged in batch format
///
/// Stored in batched format [x0, x1, .., y0, y1, .., u0, u1, .., v0, v1, ..]
///
/// \param gridArray    pointer to an array storing the grid indices
///                     2-dimensional, stored in row-major format
///                     (must have a length of nRowsV * nColsV * 4 if uv = true
///                      or nRowsV * nColsV * 2 if uv = false)
///
/// \param nRowsV       number of rows in the vertices array
/// 
/// \param nColsV       number of columns in the vertices array
/// 
/// \param xR           the x-coordinate of the right edge, defaulted to 1
///     
/// \param xL           the x-coordinate of the left edge, defaulted to -1
///     
/// \param yT           the y-coordinate of the top edge, defaulted to 1
///     
/// \param yB           the y-coordinate of the bottom edge, defaulted to -1
/// 
/// \param uv           includes UV coordinates to the array, defaulted to false
///
void array2dGridBatched(float* gridArray,
                        const int nRowsV, const int nColsV,
                        const bool uv = false,
                        const float xR = 1.0f, const float xL = -1.0f,
                        const float yT = 1.0f, const float yB = -1.0f);

#endif