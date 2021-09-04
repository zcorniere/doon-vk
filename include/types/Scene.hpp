#pragma once

#include "types/RenderObject.hpp"

#include <vector>

class Scene
{
public:
    struct DrawBatch {
        std::string meshId;
        uint32_t first;
        uint32_t count;
    };

public:
    Scene();
    ~Scene();
    inline auto getNbOfObject() const noexcept { return sceneModels.size(); }
    inline auto &getObject(auto index) { return sceneModels.at(index); }
    inline void addObject(RenderObject &&obj)
    {
        sceneModels.push_back(obj);
        bNeedRebuild = true;
    }

    void removeObject(const auto index)
    {
        sceneModels.erase(sceneModels.begin() + index);
        bNeedRebuild = true;
    }

    const std::vector<DrawBatch> &getDrawBatch(const bool bForceRebuild = false);

private:
    void buildDrawBatch();

public:
    bool bNeedRebuild = true;

private:
    std::vector<RenderObject> sceneModels;
    std::vector<DrawBatch> cachedBatch;
};
