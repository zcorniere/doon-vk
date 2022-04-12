#pragma once

#include <Logger.hpp>
#include <pivot/ecs/Components/Camera.hxx>
#include <pivot/graphics/VulkanApplication.hxx>

#include "Scene.hpp"

class Application : public pivot::graphics::VulkanApplication
{
public:
    Application();

    void init();
    void run();

    static void processKeyboard(pivot::builtins::Camera &cam, const pivot::builtins::Camera::Movement direction,
                                float dt);

public:
    glm::dvec2 last;
    bool bFirstMouse = true;
    Scene scene;
    pivot::builtins::Camera camera;
    pivot::builtins::Camera cullingCamera;
    bool cullingCameraFollowsCamera = true;
};
