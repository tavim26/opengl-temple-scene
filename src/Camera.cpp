#include "Camera.hpp"

namespace gps {

    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;

        // Calculate initial directions
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUp));
        this->cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    void Camera::move(MOVE_DIRECTION direction, float speed) {
        glm::vec3 moveDirection;
        switch (direction) {
        case MOVE_FORWARD:
            moveDirection = cameraFrontDirection;
            break;
        case MOVE_BACKWARD:
            moveDirection = -cameraFrontDirection;
            break;
        case MOVE_RIGHT:
            moveDirection = cameraRightDirection;
            break;
        case MOVE_LEFT:
            moveDirection = -cameraRightDirection;
            break;
        }
        cameraPosition += glm::normalize(moveDirection) * speed;
    }

    void Camera::rotate(float pitch, float yaw) {
        float pitchClamped = glm::clamp(pitch, -89.0f, 89.0f);
        yaw = fmod(yaw, 360.0f);
        if (yaw < 0.0f) yaw += 360.0f;

        glm::vec3 front;
        front.x = glm::cos(glm::radians(pitchClamped)) * glm::cos(glm::radians(yaw));
        front.y = glm::sin(glm::radians(pitchClamped));
        front.z = glm::cos(glm::radians(pitchClamped)) * glm::sin(glm::radians(yaw));

        this->cameraFrontDirection = glm::normalize(front);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        this->cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

}
