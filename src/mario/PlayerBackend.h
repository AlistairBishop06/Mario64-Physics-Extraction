#pragma once

#include "assets/RomImage.h"
#include "assets/RuntimeAssets.h"
#include "collision/CollisionWorld.h"
#include "mario/MarioState.h"

#include <cstdint>
#include <string>
#include <vector>

namespace sm64ps::mario {

class PlayerBackend {
public:
    virtual ~PlayerBackend() = default;

    virtual bool initialize(const assets::RomImage& rom, const collision::CollisionWorld& collisionWorld) = 0;
    virtual void shutdown() = 0;
    virtual void setCameraLook(glm::vec3 look) { (void)look; }
    virtual void tick(const MarioInput& input, float dt) = 0;
    virtual void syncBody(MarioBody& body) const = 0;
    virtual void reloadSurfaces(const collision::CollisionWorld& collisionWorld) { (void)collisionWorld; }

    virtual bool active() const = 0;
    virtual const std::string& status() const = 0;
    virtual const assets::Mesh* mesh() const { return nullptr; }
    virtual const std::vector<std::uint8_t>* textureRgba() const { return nullptr; }
    virtual int textureWidth() const { return 0; }
    virtual int textureHeight() const { return 0; }

    virtual bool initializeAudio() { return false; }
    virtual bool audioActive() const { return false; }
    virtual std::uint32_t tickAudio(std::uint32_t queuedSamples, std::uint32_t desiredSamples,
        std::vector<std::int16_t>& outSamples)
    {
        (void)queuedSamples;
        (void)desiredSamples;
        outSamples.clear();
        return 0;
    }
};

} // namespace sm64ps::mario
