#pragma once

#include "Camera.hpp"
#include "glm/vec3.hpp"

class Player : public Camera
{
public:
    static constexpr const float SPEED = 25.0f;
    static constexpr const float JUMP = 50.0f;

    static constexpr const float SENSITIVITY = 0.1f;

public:
    Player();
    ~Player();

    void update(float fElapsedTime, float fGravity) noexcept;
    void processKeyboard(const Movement direction) noexcept;
    void processMouseMovement(float xoffset, float yoffset, bool bConstrainPitch = true) noexcept;

public:
    bool isFreeFly = false;
    glm::vec3 vVelocity = {0, 0, 0};
    bool bOnFloor = true;
    float movementSpeed = SPEED;
    float jumpHeight = JUMP;
    float mouseSensitivity = SENSITIVITY;
};
