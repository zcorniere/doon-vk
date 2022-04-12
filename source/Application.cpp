#include "Application.hpp"

#include <pivot/internal/camera.hxx>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include "pivot/graphics/Renderer/CullingRenderer.hxx"
#include "pivot/graphics/Renderer/GraphicsRenderer.hxx"
#include "pivot/graphics/Renderer/ImGuiRenderer.hxx"

/// Speed movement
static constexpr const float SPEED = 0.5f;
/// Jump height
static constexpr const float JUMP = 2.5f;
/// Sensitivity of the mouse
static constexpr const float SENSITIVITY = 0.5f;
/// @cond
static constexpr const bool CONSTRAIN_PITCH = true;
/// @endcond

using namespace pivot::graphics;

Application::Application(): pivot::graphics::VulkanApplication(), camera(glm::vec3(0, 0, 0)){};

void Application::init()
{
    addRenderer<CullingRenderer>();
    addRenderer<GraphicsRenderer>();
    addRenderer<ImGuiRenderer>();
#ifdef CULLING_DEBUG
    window.setKeyPressCallback(Window::Key::C, [this](Window &window, const Window::Key key) {
        if (window.captureCursor()) this->cullingCameraFollowsCamera = !this->cullingCameraFollowsCamera;
    });
#endif
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
    VulkanApplication::init();
    //pipelineStorage.setDefault("lit");
}

void Application::run()
{
    float fElapsedTime = 0.0f;
    //pipelineStorage.setDefault("lit");
    while (!window.shouldClose()) {
        auto tp1 = std::chrono::high_resolution_clock::now();
        window.pollEvent();
        if (window.captureCursor()) {
            if (window.isKeyPressed(Window::Key::W))
                processKeyboard(camera, pivot::builtins::Camera::FORWARD, fElapsedTime);
            if (window.isKeyPressed(Window::Key::S))
                processKeyboard(camera, pivot::builtins::Camera::BACKWARD, fElapsedTime);
            if (window.isKeyPressed(Window::Key::D))
                processKeyboard(camera, pivot::builtins::Camera::RIGHT, fElapsedTime);
            if (window.isKeyPressed(Window::Key::A))
                processKeyboard(camera, pivot::builtins::Camera::LEFT, fElapsedTime);
            if (window.isKeyPressed(Window::Key::SPACE))
                processKeyboard(camera, pivot::builtins::Camera::UP, fElapsedTime);
            if (window.isKeyPressed(Window::Key::LEFT_SHIFT))
                processKeyboard(camera, pivot::builtins::Camera::DOWN, fElapsedTime);
        }
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Test Window");
            ImGui::Text("Fps: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("ms/frame %.3f", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Separator();
#ifdef CULLING_DEBUG
            ImGui::Text("Culling separated from camera: %s", std::to_string(!cullingCameraFollowsCamera).c_str());
#endif
            ImGui::End();

            ImGui::Render();
        }
#ifdef CULLING_DEBUG
        if (cullingCameraFollowsCamera) cullingCamera = camera;
#endif
        draw(scene.getSceneInformations(), pivot::internals::getGPUCameraData(camera, 80.0f, getAspectRatio())
#ifdef CULLING_DEBUG
                                               ,
             std::make_optional(pivot::internals::getGPUCameraData(cullingCamera, 80.0f, getAspectRatio()))
#endif
        );
        auto tp2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsedTime(tp2 - tp1);
        fElapsedTime = elapsedTime.count();
    }
}

void Application::processKeyboard(pivot::builtins::Camera &cam, const pivot::builtins::Camera::Movement direction,
                                  float dt)
{
    switch (direction) {
        case pivot::builtins::Camera::Movement::FORWARD: {
            cam.position.x += cam.front.x * SPEED * (dt * 500);
            cam.position.z += cam.front.z * SPEED * (dt * 500);
        } break;
        case pivot::builtins::Camera::Movement::BACKWARD: {
            cam.position.x -= cam.front.x * SPEED * (dt * 500);
            cam.position.z -= cam.front.z * SPEED * (dt * 500);
        } break;
        case pivot::builtins::Camera::Movement::RIGHT: {
            cam.position.x += cam.right.x * SPEED * (dt * 500);
            cam.position.z += cam.right.z * SPEED * (dt * 500);
        } break;
        case pivot::builtins::Camera::Movement::LEFT: {
            cam.position.x -= cam.right.x * SPEED * (dt * 500);
            cam.position.z -= cam.right.z * SPEED * (dt * 500);
        } break;
        case pivot::builtins::Camera::Movement::UP: {
            cam.position.y += JUMP * (dt * 500);
        } break;
        case pivot::builtins::Camera::Movement::DOWN: cam.position.y -= SPEED * (dt * 500); break;
    }
}
