#pragma once

#include <pivot/graphics/types/RenderObject.hxx>
#include <vector>

class Scene
{
public:
    Scene() = default;
    void addObject(RenderObject o);

    constexpr std::vector<std::reference_wrapper<const RenderObject>> &getSceneInformations() noexcept
    {
        return obj_ref;
    }

private:
    std::vector<std::reference_wrapper<const RenderObject>> obj_ref;
    std::vector<RenderObject> obj;
};
