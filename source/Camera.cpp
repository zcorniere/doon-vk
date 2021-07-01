#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
Camera::Camera(glm::vec3 pos, glm::vec3 up, float yaw, float pitch)
    : position(pos),
      front(glm::vec3(0.0f, 0.0f, -1.0f)),
      worldUp(up),
      yaw(yaw),
      pitch(pitch),
      movementSpeed(SPEED),
      mouseSensitivity(SENSITIVITY)
{
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f)), yaw(yaw), pitch(pitch), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY)
{
    position = glm::vec3(posX, posY, posZ);
    worldUp = glm::vec3(upX, upY, upZ);
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const { return glm::lookAt(position, position + front, up); }

Camera::GPUCameraData Camera::getGPUCameraData() const
{
    auto projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    projection[1][1] *= -1;
    auto view = this->getViewMatrix();
    GPUCameraData data{
        .position = glm::vec4(position, 1.0f),
        .viewproj = projection * view,
    };
    return data;
}

void Camera::processKeyboard(Movement direction, float fElapsedTime)
{
    float velocity = movementSpeed * fElapsedTime;
    switch (direction) {
        case Movement::FORWARD: position += front * velocity; break;
        case Movement::BACKWARD: position -= front * velocity; break;
        case Movement::RIGHT: position += right * velocity; break;
        case Movement::LEFT: position -= right * velocity; break;
        case Movement::UP: position += up * velocity; break;
        case Movement::DOWN: position -= up * velocity; break;
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool bConstrainPitch)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (bConstrainPitch) {
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    glm::vec3 tmpFront;
    tmpFront.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    tmpFront.y = std::sin(glm::radians(pitch));
    tmpFront.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

    front = glm::normalize(tmpFront);
    right = glm::normalize(glm::cross(tmpFront, worldUp));
    up = glm::normalize(glm::cross(right, tmpFront));
}

std::ostream &operator<<(std::ostream &os, const Camera &camera)
{
    os << "Camera (x = " << camera.position.x << ", y = " << camera.position.y << ", x = " << camera.position.z << ")";
    return os;
}