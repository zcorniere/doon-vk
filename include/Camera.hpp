#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <ostream>

#ifndef MAX_PROJECTION_LIMIT
#define MAX_PROJECTION_LIMIT 100.0f
#endif

class Camera
{
public:
    struct GPUCameraData {
        glm::vec4 position;
        glm::mat4 viewproj;
    };
    enum Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };
    static constexpr const float YAW = -90.0f;
    static constexpr const float PITCH = 0.0f;

public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = Camera::YAW, float pitch = Camera::PITCH);
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);
    glm::mat4 getViewMatrix() const;
    GPUCameraData getGPUCameraData(float fFOV = 70.f, float fAspectRatio = 1700.f / 900.f,
                                   float fCloseClippingPlane = 0.1,
                                   float fFarClippingPlane = MAX_PROJECTION_LIMIT) const;

protected:
    void updateCameraVectors();

public:
    glm::vec3 vVelocity = {0, 0, 0};
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
};

std::ostream &operator<<(std::ostream &, const Camera &);
