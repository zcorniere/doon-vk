#pragma once

#include <pivot/graphics/types/RenderObject.hxx>
#include <vector>

class Scene
{
public:
    Scene() = default;
    void addObject(RenderObject o);

    constexpr const std::vector<std::reference_wrapper<const RenderObject>> &getSceneInformations() const noexcept
    {
        return obj_ref;
    }

private:
    std::vector<std::reference_wrapper<const RenderObject>> obj_ref;
    std::vector<RenderObject> obj;
};