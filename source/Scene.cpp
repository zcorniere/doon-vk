#include "Scene.hpp"

#include <functional>

void Scene::addObject(RenderObject o)
{
    obj.push_back(std::move(o));
    obj_ref.clear();
    for (const auto &o: obj) obj_ref.push_back(o);
}
