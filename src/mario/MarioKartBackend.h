#pragma once

#include "mario/PlayerBackend.h"

namespace sm64ps::mario {

class MarioKartBackend final : public PlayerBackend {
public:
    bool initialize(const assets::RomImage& rom, const collision::CollisionWorld& collisionWorld) override;
    void shutdown() override;
    void setCameraLook(glm::vec3 look) override;
    void tick(const MarioInput& input, float dt) override;
    void syncBody(MarioBody& body) const override;
    void reloadSurfaces(const collision::CollisionWorld& collisionWorld) override;

    bool active() const override { return active_; }
    const std::string& status() const override { return status_; }
    const assets::Mesh* mesh() const override { return &mesh_; }

private:
    void buildMesh();

    bool active_ = false;
    std::string status_ = "Mario Kart 64 backend not initialized";
    const collision::CollisionWorld* collisionWorld_ = nullptr;
    MarioBody body_;
    assets::Mesh mesh_;
    glm::vec3 cameraLook_ { 0.0f, 0.0f, 1.0f };
    float yawRadians_ = 0.0f;
    float speed_ = 0.0f;
    float hopTimer_ = 0.0f;
};

} // namespace sm64ps::mario
