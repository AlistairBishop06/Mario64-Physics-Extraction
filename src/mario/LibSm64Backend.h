#pragma once

#include "mario/PlayerBackend.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sm64ps::mario {

class LibSm64Backend final : public PlayerBackend {
public:
    ~LibSm64Backend();

    bool initialize(const assets::RomImage& rom, const collision::CollisionWorld& collisionWorld) override;
    bool initialize(const std::filesystem::path& dllPath, const std::filesystem::path& romPath,
        const collision::CollisionWorld& collisionWorld);
    bool initialize(const std::filesystem::path& dllPath, const assets::RomImage& rom,
        const collision::CollisionWorld& collisionWorld);
    void shutdown() override;
    void setCameraLook(glm::vec3 look) override;
    void tick(const MarioInput& input, float dt) override;
    void syncBody(MarioBody& body) const override;
    void reloadSurfaces(const collision::CollisionWorld& collisionWorld) override;
    bool initializeAudio() override;
    std::uint32_t tickAudio(std::uint32_t queuedSamples, std::uint32_t desiredSamples,
        std::vector<std::int16_t>& outSamples) override;

    bool active() const override { return active_; }
    bool audioActive() const override { return audioActive_; }
    const std::string& status() const override { return status_; }
    const assets::Mesh* mesh() const override { return &mesh_; }
    const std::vector<std::uint8_t>* textureRgba() const override { return &textureRgba_; }
    int textureWidth() const override;
    int textureHeight() const override;

    static std::filesystem::path findDefaultLibrary();
    static std::filesystem::path findDefaultRom();

private:
    struct Api;

    bool loadLibrary(const std::filesystem::path& dllPath);
    bool bindApi();
    void unloadLibrary();
    void loadSurfaces(const collision::CollisionWorld& collisionWorld);

    void* library_ = nullptr;
    Api* api_ = nullptr;
    bool active_ = false;
    bool audioActive_ = false;
    int marioId_ = -1;
    std::string status_ = "libsm64 backend not initialized";

    std::vector<std::uint8_t> romBytes_;
    std::vector<std::uint8_t> textureRgba_;
    assets::Mesh mesh_;
    glm::vec3 lastPosition_ { 0.0f };
    glm::vec3 lastVelocity_ { 0.0f };
    float lastFaceAngle_ = 0.0f;
    glm::vec3 cameraLook_ { 0.0f, 0.0f, 1.0f };

    std::vector<float> positions_;
    std::vector<float> normals_;
    std::vector<float> colors_;
    std::vector<float> uvs_;
};

} // namespace sm64ps::mario
