#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 pos, glm::vec3 up, float yaw, float pitch)
    : position(pos), front(glm::vec3(0.0f, 0.0f, -1.0f)), worldUp(up), yaw(yaw), pitch(pitch)
{
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f)), yaw(yaw), pitch(pitch)
{
    position = glm::vec3(posX, posY, posZ);
    worldUp = glm::vec3(upX, upY, upZ);
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const { return glm::lookAt(position, position + front, up); }

Camera::GPUCameraData Camera::getGPUCameraData(float fFOV, float fAspectRatio, float fCloseClippingPlane,
                                               float fFarClippingPlane) const
{
    auto projection = glm::perspective(glm::radians(fFOV), fAspectRatio, fCloseClippingPlane, fFarClippingPlane);
    projection[1][1] *= -1;
    auto view = this->getViewMatrix();
    GPUCameraData data{
        .position = glm::vec4(position, 1.0f),
        .viewproj = projection * view,
    };
    return data;
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
