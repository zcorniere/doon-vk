#include "types/Scene.hpp"

Scene::Scene() {}

Scene::~Scene() {}

const std::vector<Scene::DrawBatch> &Scene::getDrawBatch(const bool bForceRebuild)
{
    if (bNeedRebuild || bForceRebuild) { this->buildDrawBatch(); }
    return cachedBatch;
}

void Scene::buildDrawBatch()
{
    std::sort(sceneModels.begin(), sceneModels.end(),
              [](const auto &first, const auto &second) { return first.meshID == second.meshID; });
    cachedBatch.clear();
    cachedBatch.push_back({
        .meshId = sceneModels.at(0).meshID,
        .first = 0,
        .count = 1,
    });

    for (uint32_t i = 1; i < sceneModels.size(); i++) {
        if (sceneModels[i].meshID == cachedBatch.back().meshId) {
            cachedBatch.back().count++;
        } else {
            cachedBatch.push_back({
                .meshId = sceneModels[i].meshID,
                .first = i,
                .count = 1,
            });
        }
    }
    bNeedRebuild = false;
}
