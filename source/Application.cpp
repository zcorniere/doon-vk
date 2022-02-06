#include "Application.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

/// Speed movement
static constexpr const float SPEED = 0.5f;
/// Jump height
static constexpr const float JUMP = 2.5f;
/// Sensitivity of the mouse
static constexpr const float SENSITIVITY = 0.5f;
/// @cond
static constexpr const bool CONSTRAIN_PITCH = true;
/// @endcond

Application::Application(): pivot::graphics::VulkanApplication(), camera(glm::vec3(0, 0, 0)){};

void Application::init()
{
    assetStorage.loadModels("../assets/sponza/sponza.gltf");    //, "../models/sponza/sponza2.obj");
    assetStorage.loadTextures("../textures/grey.png");

    window.setKeyPressCallback(Window::Key::LEFT_ALT, [&](Window &window, const Window::Key key) {
        window.captureCursor(!window.captureCursor());
        bFirstMouse = true;
    });
    window.setMouseMovementCallback([&](Window &window, const glm::dvec2 pos) {
        if (!window.captureCursor()) return;
        if (bFirstMouse) {
            last = pos;
            bFirstMouse = false;
        }
        auto offset = glm::dvec2(pos.x - last.x, last.y - pos.y);
        last = pos;
        camera.yaw += offset.x * SENSITIVITY;
        camera.pitch += offset.y * SENSITIVITY;
        if (CONSTRAIN_PITCH) {
            if (camera.pitch > 89.0f) camera.pitch = 89.0f;
            if (camera.pitch < -89.0f) camera.pitch = -89.0f;
        }
        camera.updateCameraVectors();
    });
    scene.addObject({
        .meshID = "sponza",
        .objectInformation =
            {
                .transform = Transform({}, {}, glm::vec3(170)),
            },
    });
    // scene.addObject({
    //     .meshID = "sponza2",
    //     .objectInformation =
    //         {
    //             .transform = Transform(glm::vec3(0, 0, 3000), {}, glm::vec3(1)),
    //         },
    // });
    // scene.addObject({.meshID = "basic"});
}

void Application::run()
{
    float fElapsedTime = 0.0f;
    VulkanApplication::init();
    while (!window.shouldClose()) {
        auto tp1 = std::chrono::high_resolution_clock::now();
        window.pollEvent();
        if (window.captureCursor()) {
            if (window.isKeyPressed(Window::Key::W)) processKeyboard(camera, Camera::FORWARD, fElapsedTime);
            if (window.isKeyPressed(Window::Key::S)) processKeyboard(camera, Camera::BACKWARD, fElapsedTime);
            if (window.isKeyPressed(Window::Key::D)) processKeyboard(camera, Camera::RIGHT, fElapsedTime);
            if (window.isKeyPressed(Window::Key::A)) processKeyboard(camera, Camera::LEFT, fElapsedTime);
            if (window.isKeyPressed(Window::Key::SPACE)) processKeyboard(camera, Camera::UP, fElapsedTime);
            if (window.isKeyPressed(Window::Key::LEFT_SHIFT)) processKeyboard(camera, Camera::DOWN, fElapsedTime);
        }
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Test Window");
            ImGui::Text("Fps: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("ms/frame %.3f", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::End();

            ImGui::Render();
        }
        draw(scene.getSceneInformations(), camera.getGPUCameraData(80.0f, getAspectRatio()));
        auto tp2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedTime(tp2 - tp1);
        fElapsedTime = elapsedTime.count();
    }
}

void Application::processKeyboard(Camera &cam, const Camera::Movement direction, float dt)
{
    switch (direction) {
        case Camera::Movement::FORWARD: {
            cam.position.x += cam.front.x * SPEED * (dt * 500);
            cam.position.z += cam.front.z * SPEED * (dt * 500);
        } break;
        case Camera::Movement::BACKWARD: {
            cam.position.x -= cam.front.x * SPEED * (dt * 500);
            cam.position.z -= cam.front.z * SPEED * (dt * 500);
        } break;
        case Camera::Movement::RIGHT: {
            cam.position.x += cam.right.x * SPEED * (dt * 500);
            cam.position.z += cam.right.z * SPEED * (dt * 500);
        } break;
        case Camera::Movement::LEFT: {
            cam.position.x -= cam.right.x * SPEED * (dt * 500);
            cam.position.z -= cam.right.z * SPEED * (dt * 500);
        } break;
        case Camera::Movement::UP: {
            cam.position.y += JUMP * (dt * 500);
        } break;
        case Camera::Movement::DOWN: cam.position.y -= SPEED * (dt * 500); break;
    }
}
