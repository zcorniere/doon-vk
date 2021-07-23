#pragma once

#include "Camera.hpp"

class Player : public Camera
{
public:
    static constexpr const float SPEED = 25.0f;
    static constexpr const float JUMP = 50.0f;

    static constexpr const float SENSITIVITY = 0.1f;

public:
    Player();
    ~Player();

    void update(float fElapsedTime, float fGravity);
    void processKeyboard(Movement direction);
    void processMouseMovement(float xoffset, float yoffset, bool bConstrainPitch = true);

public:
    glm::vec3 vVelocity = {0, 0, 0};
    bool bOnFloor = true;
    float movementSpeed = SPEED;
    float jumpHeight = JUMP;
    float mouseSensitivity = SENSITIVITY;
};
