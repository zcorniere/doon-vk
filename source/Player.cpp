#include "Player.hpp"

#include <ostream>
#include <string>

#include "Logger.hpp"
#include "glm/vec3.hpp"

Player::Player(): Camera() {}

Player::~Player() {}

void Player::update(float fElapsedTime, float fGravity) noexcept
{
    if (fGravity > 0) {
        LOGGER_WARN << "fGravity is positive ! : " << fGravity;
        LOGGER_ENDL;
    }
    position += vVelocity * fElapsedTime;
    vVelocity.x = 0;
    vVelocity.z = 0;
    vVelocity.y += (isFreeFly) ? (-vVelocity.y) : (fGravity);

    if (position.y < 6 && !isFreeFly) {
        position.y = 6;
        vVelocity = glm::vec3(0);
        bOnFloor = true;
    }
}

void Player::processKeyboard(const Movement direction) noexcept
{
    auto velocity = (isFreeFly) ? (50) : (movementSpeed / ((bOnFloor == true) ? (1) : (1.1)));
    switch (direction) {
        case Movement::FORWARD: {
            vVelocity.x += front.x * velocity;
            vVelocity.z += front.z * velocity;
        } break;
        case Movement::BACKWARD: {
            vVelocity.x -= front.x * velocity;
            vVelocity.z -= front.z * velocity;
        } break;
        case Movement::RIGHT: {
            vVelocity.x += right.x * velocity;
            vVelocity.z += right.z * velocity;
        } break;
        case Movement::LEFT: {
            vVelocity.x -= right.x * velocity;
            vVelocity.z -= right.z * velocity;
        } break;
        case Movement::UP: {
            if (bOnFloor || isFreeFly) {
                bOnFloor = false;
                vVelocity.y += jumpHeight;
            }
        } break;
        case Movement::DOWN: vVelocity.y -= velocity; break;
    }
}

void Player::processMouseMovement(float xoffset, float yoffset, bool bConstrainPitch) noexcept
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
