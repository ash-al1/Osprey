#include "camera.hpp"

#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL // for Euler angles
#include <glm/gtx/euler_angles.hpp>


Camera::Camera(double sensitivity,
               double rightSensitivity,
               double epsilon, 
               double scrollSensitivity)
    :   sensitivity(sensitivity), 
        rightSensitivity(rightSensitivity),
        epsilon(epsilon),
        scrollSensitivity(scrollSensitivity),

        dragging(false),
        lastX(0.0), lastY(0.0),
        dx(0.0), dy(0.0), 

        rDragging(false),
        rLastX(0.0), rLastY(0.0),
        rDx(0.0), rDy(0.0), 

        defaultAlphaAngle(0.0), 
        defaultBetaAngle(0.0), 
        defaultScalingFactor(1.0),
        defaultTranslationVec(glm::vec3(0.0f)),

        undoEnable(false),
        lastDragging(false),
        lastRightDragging(false),
        lastScrolling(false),
        lastAlphaAngle(defaultAlphaAngle), 
        lastBetaAngle(defaultBetaAngle), 
        lastScalingFactor(defaultScalingFactor),
        lastTranslationVec(defaultTranslationVec),

        redoEnable(false),
        redoAlphaAngle(defaultAlphaAngle), 
        redoBetaAngle(defaultBetaAngle), 
        redoScalingFactor(defaultScalingFactor),
        redoTranslationVec(defaultTranslationVec),

        scrolling(false),
        startScrollingTime(std::chrono::high_resolution_clock::now()),
        lastScrollingTime(std::chrono::high_resolution_clock::now()),

        alphaAngle(defaultAlphaAngle), 
        betaAngle(defaultBetaAngle), 
        scalingFactor(defaultScalingFactor),

        translationVec(glm::vec3(0)),
        rotationMat(glm::eulerAngleYX(alphaAngle, betaAngle)){}

void Camera::returnButton()
{
    // undo to state before hitting the return button
    this->enableUndo();

    // return to default state
    this->alphaAngle = defaultAlphaAngle;
    this->betaAngle = defaultBetaAngle;
    this->rotationMat = glm::eulerAngleYX(alphaAngle, betaAngle);
    this->scalingFactor = defaultScalingFactor;
    this->translationVec = defaultTranslationVec;

}

void Camera::undoButton()
{
    if (undoEnable)
    {
        // storing redo states
        this->redoAlphaAngle = alphaAngle;
        this->redoBetaAngle = betaAngle;   
        this->redoScalingFactor = scalingFactor;
        this->redoTranslationVec = translationVec;

        // undo to last state
        this->alphaAngle = lastAlphaAngle;
        this->betaAngle = lastBetaAngle;
        this->rotationMat = glm::eulerAngleYX(alphaAngle, betaAngle);
        this->scalingFactor = lastScalingFactor;
        this->translationVec = lastTranslationVec;

        // update buttons state
        this-> redoEnable = true;
        this-> undoEnable = false;
    }
}

void Camera::redoButton()
{
    if (redoEnable) // redoEnable > 0 if multi-undo is implemented
    {
        // update buttons state
        this->undoEnable = true;
        this->redoEnable = false;

        this->alphaAngle = redoAlphaAngle;
        this->betaAngle = redoBetaAngle;
        this->rotationMat = glm::eulerAngleYX(alphaAngle, betaAngle);
        this->scalingFactor = redoScalingFactor;
        this->translationVec = redoTranslationVec;

    }
}

void Camera::enableUndo()
{
    this->lastAlphaAngle = alphaAngle;
    this->lastBetaAngle = betaAngle;
    this->lastScalingFactor = scalingFactor;
    this->lastTranslationVec = translationVec;

    // update button states
    this->undoEnable = true;
    // disable redo button since state has changed
    this->redoEnable = false;    
}

void Camera::drag2Rotate(double x, double y, bool clicking)
{
    
    if (clicking)
    {
        if (!dragging)
        {
            this->lastX = x;
            this->lastY = y;
            this->dragging = true;
        }
        else
        {
            dx = x - lastX;
            dy = y - lastY;

            if (fabs(dx) >= epsilon || fabs(dy) >= epsilon)
            {
                // this->alphaAngle += sensitivity * dx;
                // this->betaAngle += sensitivity * dy;
                this->alphaAngle -= sensitivity * dx;
                this->betaAngle -= sensitivity * dy;
                this->rotationMat = glm::eulerAngleYX(alphaAngle, betaAngle);
                this->lastX = x;
                this->lastY = y;
            }
        }
    }
    else if (dragging)
    {
        this->dragging = false;
    }
}

void Camera::rightDrag2Move(double x, double y, bool clicking)
{
    
    if (clicking)
    {
        if (!rDragging)
        {
            this->rLastX = x;
            this->rLastY = y;
            this->rDragging = true;
        }
        else
        {
            rDx = x - rLastX;
            rDy = y - rLastY;

            if (fabs(rDx) >= epsilon || fabs(rDy) >= epsilon)
            {
                this->translationVec += glm::vec3(rightSensitivity * rDx, 
                                                  -rightSensitivity * rDy, 
                                                  0);
                this->rLastX = x;
                this->rLastY = y;
            }
        }
    }
    else if (rDragging)
    {
        this->rDragging = false;
    }
}

void Camera::scroll2Zoom(double yoffset, int timeout)
{
    // update undo button state
    if (!scrolling)
    {
        this->enableUndo();

        auto currentTime = std::chrono::high_resolution_clock::now();
        if (currentTime - lastScrollingTime >= std::chrono::milliseconds(timeout))
        {
            this->startScrollingTime = currentTime;
        }
    }

    this->isScrolling();
    // update scaling factor
    this->scalingFactor *= 1.0 + scrollSensitivity * yoffset;
}

void Camera::isScrolling()
{
    this->scrolling = true;
    this->lastScrollingTime = std::chrono::high_resolution_clock::now();
}

void Camera::isNotScrolling(int timeout)
{
    auto currentTime = std::chrono::high_resolution_clock::now(); 
    
    if (currentTime - startScrollingTime > std::chrono::milliseconds(timeout))
    {
        this->scrolling = false;
    }
}

/// BUG: Implement undo button for right click and drag
glm::mat4 Camera::getPVMMat()
{
    // undo button states
    // ------------------
    // since getPVMMat() will be ran on every frame
    // we can use it to update the last state for the undo button

    // rotation
    if (!lastDragging && dragging)
    {
        this->enableUndo();
    }

    this->lastDragging = dragging;

    // translation
    if (!lastRightDragging && rDragging)
    {
        this->enableUndo();
    }

    this->lastRightDragging = rDragging;

    // actually getting the PVM matrix
    // -------------------------------
    // scaling
    glm::mat4 transform = glm::scale(rotationMat, glm::vec3(scalingFactor));
    
    // apply translation to 4th column
    transform[3] = glm::vec4(translationVec, 1.0f);
    
    return transform;
}