#pragma once

#include "assets/RuntimeAssets.h"
#include "collision/CollisionWorld.h"
#include "mario/MarioState.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sm64ps::mario {

class LibSm64Backend {
public:
    ~LibSm64Backend();

    bool initialize(const std::filesystem::path& dllPath, const std::filesystem::path& romPath,
        const collision::CollisionWorld& collisionWorld);
    void shutdown();
    void setCameraLook(glm::vec3 look);
    void tick(const MarioInput& input);
    void syncBody(MarioBody& body) const;
    bool initializeAudio();
    std::uint32_t tickAudio(std::uint32_t queuedSamples, std::uint32_t desiredSamples, std::vector<std::int16_t>& outSamples);

    bool active() const { return active_; }
    bool audioActive() const { return audioActive_; }
    const std::string& status() const { return status_; }
    const assets::Mesh& mesh() const { return mesh_; }
    const std::vector<std::uint8_t>& textureRgba() const { return textureRgba_; }

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
