//===----------------------------------------------------------------------===//
//
// Camera class for manipulating the viewport
// 
//===----------------------------------------------------------------------===//

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <chrono>

class Camera
{
public:
    Camera(double sensitivity = 5e-3, 
           double rightSensitivity = 2e-3,
           double epsilon = 1e-5,
           double scrollSensitivity = 5e-2);

    /// Click return button to return to default state
    void returnButton();

    /// Click undo button to return to the last state
    /// Currently can only undo once
    void undoButton();

    /// Click redo button to return to the state before undo
    void redoButton();

    /// Drag left mouse button to rotate the viewport
    ///
    /// \param x        the current x-location
    /// \param y        the current y-location
    /// \param clicking whether the user is left clicking the mouse
    void drag2Rotate(double x, double y, bool clicking);

    /// Drag right mouse button to rotate the viewport
    ///
    /// \param x        the current x-location
    /// \param y        the current y-location
    /// \param clicking whether the user is left clicking the mouse
    void rightDrag2Move(double x, double y, bool clicking);

    /// Scroll to zoom
    ///
    /// \param yoffset  mouse wheel scroll offset
    void scroll2Zoom(double yoffset, int timeout = 100);

    /// Record scrolling event for the undo button
    void isScrolling();

    /// Record "un"-scrolling for the redo button
    ///
    /// \param timeout  time requires for scrolling state to change from 
    ///                 true to false, in milliseconds
    void isNotScrolling(int timeout = 100);

    /// Get the PVM matrix
    /// Also use for updating the last camera state for the undo button
    glm::mat4 getPVMMat();

private:
    // settings
    double sensitivity;
    double rightSensitivity;
    double epsilon;
    double scrollSensitivity;

    // attributes
    // ----------
    bool dragging;
    double lastX, lastY;
    double dx, dy;
    bool rDragging;
    double rLastX, rLastY;
    double rDx, rDy;

    // return button
    double defaultAlphaAngle;
    double defaultBetaAngle;
    double defaultScalingFactor;
    glm::vec3 defaultTranslationVec;

    // undo button
    bool undoEnable;
    bool lastDragging;
    bool lastRightDragging;
    bool lastScrolling;
    double lastAlphaAngle;
    double lastBetaAngle;
    double lastScalingFactor;
    glm::vec3 lastTranslationVec;

    // redo button
    bool redoEnable;
    double redoAlphaAngle;
    double redoBetaAngle;
    double redoScalingFactor;
    glm::vec3 redoTranslationVec;

    // scrolling
    bool scrolling;
    std::chrono::time_point<std::chrono::high_resolution_clock> startScrollingTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastScrollingTime;

    // current state
    double alphaAngle;
    double betaAngle;
    double scalingFactor;

    glm::vec3 translationVec;
    glm::mat4 rotationMat;

    /// Activates the undo button, storing undo states, disable redo button.
    void enableUndo();
};

#endif