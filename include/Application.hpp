#pragma once

#include <pivot/ecs/Components/Camera.hxx>
#include <pivot/graphics/VulkanApplication.hxx>
#include <pivot/graphics/Window.hxx>
#include <pivot/graphics/vk_utils.hxx>

#include <Logger.hpp>

#include "Scene.hpp"

class Application : public pivot::graphics::VulkanApplication
{
public:
    Application();

    void init();
    void run();

    static void processKeyboard(Camera &cam, const Camera::Movement direction, float dt);

public:
    glm::dvec2 last;
    bool bFirstMouse = true;
    Scene scene;
    Camera camera;
    Camera cullingCamera;
    bool cullingCameraFollowsCamera = true;
};
