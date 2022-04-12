#pragma once

#include <pivot/graphics/types/RenderObject.hxx>
#include <vector>

class Scene
{
public:
    Scene() = default;
    void addObject(pivot::graphics::RenderObject o);

    constexpr std::vector<std::reference_wrapper<const pivot::graphics::RenderObject>> &getSceneInformations() noexcept
    {
        return obj_ref;
    }

private:
    std::vector<std::reference_wrapper<const pivot::graphics::RenderObject>> obj_ref;
    std::vector<pivot::graphics::RenderObject> obj;
};
