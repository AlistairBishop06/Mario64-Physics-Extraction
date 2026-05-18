#include "mario/LibSm64Backend.h"

#include "util/Log.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace sm64ps::mario {

namespace {

constexpr int kTextureWidth = 64 * 11;
constexpr int kTextureHeight = 64;
constexpr int kMaxTriangles = 1024;

struct SM64Surface {
    std::int16_t type;
    std::int16_t force;
    std::uint16_t terrain;
    std::int32_t vertices[3][3];
};

struct SM64MarioInputs {
    float camLookX;
    float camLookZ;
    float stickX;
    float stickY;
    std::uint8_t buttonA;
    std::uint8_t buttonB;
    std::uint8_t buttonZ;
};

struct SM64MarioState {
    float position[3];
    float velocity[3];
    float faceAngle;
    float forwardVelocity;
    std::int16_t health;
    std::uint32_t action;
    std::int32_t animID;
    std::int16_t animFrame;
    std::uint32_t flags;
    std::uint32_t particleFlags;
    std::int16_t invincTimer;
};

struct SM64MarioGeometryBuffers {
    float* position;
    float* normal;
    float* color;
    float* uv;
    std::uint16_t numTrianglesUsed;
};

std::vector<std::uint8_t> readAllBytes(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

std::int32_t toSm64Coord(float value)
{
    return static_cast<std::int32_t>(std::lround(value));
}

void* loadSymbol(void* library, const char* name)
{
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name));
#else
    return dlsym(library, name);
#endif
}

} // namespace

struct LibSm64Backend::Api {
    using GlobalInit = void (*)(const std::uint8_t*, std::uint8_t*);
    using GlobalTerminate = void (*)();
    using StaticSurfacesLoad = void (*)(const SM64Surface*, std::uint32_t);
    using MarioCreate = std::int32_t (*)(float, float, float);
    using MarioTick = void (*)(std::int32_t, const SM64MarioInputs*, SM64MarioState*, SM64MarioGeometryBuffers*);
    using MarioDelete = void (*)(std::int32_t);

    GlobalInit globalInit = nullptr;
    GlobalTerminate globalTerminate = nullptr;
    StaticSurfacesLoad staticSurfacesLoad = nullptr;
    MarioCreate marioCreate = nullptr;
    MarioTick marioTick = nullptr;
    MarioDelete marioDelete = nullptr;
};

LibSm64Backend::~LibSm64Backend()
{
    shutdown();
}

bool LibSm64Backend::initialize(const std::filesystem::path& dllPath, const std::filesystem::path& romPath,
    const collision::CollisionWorld& collisionWorld)
{
    shutdown();

    if (dllPath.empty() || !std::filesystem::exists(dllPath)) {
        status_ = "libsm64 DLL not found. Build libsm64 and place sm64.dll under third_party/libsm64/dist/.";
        return false;
    }

    romBytes_ = readAllBytes(romPath);
    if (romBytes_.empty()) {
        status_ = "SM64 US ROM not found/readable for libsm64 runtime.";
        return false;
    }

    if (!loadLibrary(dllPath) || !bindApi()) {
        status_ = "Failed to load libsm64 symbols from " + dllPath.string();
        unloadLibrary();
        return false;
    }

    textureRgba_.assign(kTextureWidth * kTextureHeight * 4, 255);
    api_->globalInit(romBytes_.data(), textureRgba_.data());
    loadSurfaces(collisionWorld);
    marioId_ = api_->marioCreate(0.0f, 0.0f, 0.0f);

    positions_.assign(kMaxTriangles * 3 * 3, 0.0f);
    normals_.assign(kMaxTriangles * 3 * 3, 0.0f);
    colors_.assign(kMaxTriangles * 3 * 3, 1.0f);
    uvs_.assign(kMaxTriangles * 3 * 2, 0.0f);

    active_ = marioId_ >= 0;
    status_ = active_
        ? "Using libsm64: real ROM-backed Mario movement, animation, and geometry."
        : "libsm64 loaded but Mario instance creation failed.";
    util::logInfo(status_);
    return active_;
}

void LibSm64Backend::shutdown()
{
    if (api_ && active_ && marioId_ >= 0) {
        api_->marioDelete(marioId_);
    }
    if (api_) {
        api_->globalTerminate();
    }
    marioId_ = -1;
    active_ = false;
    delete api_;
    api_ = nullptr;
    unloadLibrary();
}

void LibSm64Backend::setCameraLook(glm::vec3 look)
{
    look.y = 0.0f;
    if (glm::length(look) > 0.001f) {
        cameraLook_ = glm::normalize(look);
    }
}

void LibSm64Backend::tick(const MarioInput& input)
{
    if (!active_ || !api_) {
        return;
    }

    SM64MarioInputs sm64Input {};
    sm64Input.camLookX = cameraLook_.x;
    sm64Input.camLookZ = cameraLook_.z;
    sm64Input.stickX = std::clamp(input.stick.x, -1.0f, 1.0f);
    // libsm64 follows the original controller convention: forward is negative Y.
    sm64Input.stickY = std::clamp(-input.stick.y, -1.0f, 1.0f);
    sm64Input.buttonA = input.jumpPressed ? 1 : 0;
    sm64Input.buttonB = input.attackPressed ? 1 : 0;
    sm64Input.buttonZ = input.crouchHeld ? 1 : 0;

    SM64MarioState state {};
    SM64MarioGeometryBuffers buffers {};
    buffers.position = positions_.data();
    buffers.normal = normals_.data();
    buffers.color = colors_.data();
    buffers.uv = uvs_.data();

    api_->marioTick(marioId_, &sm64Input, &state, &buffers);

    mesh_.name = "libsm64_mario";
    mesh_.worldSpace = true;
    mesh_.vertices.clear();
    mesh_.vertices.reserve(static_cast<std::size_t>(buffers.numTrianglesUsed) * 3);

    const std::size_t vertexCount = static_cast<std::size_t>(buffers.numTrianglesUsed) * 3;
    for (std::size_t i = 0; i < vertexCount; ++i) {
        assets::MeshVertex vertex;
        vertex.position = {
            positions_[i * 3 + 0],
            positions_[i * 3 + 1],
            positions_[i * 3 + 2],
        };
        vertex.normal = {
            normals_[i * 3 + 0],
            normals_[i * 3 + 1],
            normals_[i * 3 + 2],
        };
        vertex.color = {
            colors_[i * 3 + 0],
            colors_[i * 3 + 1],
            colors_[i * 3 + 2],
            1.0f,
        };
        vertex.uv = {
            uvs_[i * 2 + 0],
            uvs_[i * 2 + 1],
        };
        mesh_.vertices.push_back(vertex);
    }

    lastPosition_ = { state.position[0], state.position[1], state.position[2] };
    lastVelocity_ = { state.velocity[0], state.velocity[1], state.velocity[2] };
    lastFaceAngle_ = state.faceAngle;
}

void LibSm64Backend::syncBody(MarioBody& body) const
{
    if (!active_) {
        return;
    }
    body.position = lastPosition_;
    body.velocity = lastVelocity_;
    body.faceYaw = radiansToAngle16(lastFaceAngle_);
    body.grounded = std::abs(lastVelocity_.y) < 0.01f;
    body.action = body.grounded ? Action::Idle : Action::Freefall;
}

std::filesystem::path LibSm64Backend::findDefaultLibrary()
{
#if defined(_WIN32)
    const std::array<std::filesystem::path, 5> candidates {
        "third_party/libsm64/dist/sm64.dll",
        "external/libsm64/dist/sm64.dll",
        "libsm64/dist/sm64.dll",
        "build/Release/sm64.dll",
        "sm64.dll",
    };
#else
    const std::array<std::filesystem::path, 4> candidates {
        "third_party/libsm64/dist/libsm64.so",
        "external/libsm64/dist/libsm64.so",
        "libsm64/dist/libsm64.so",
        "libsm64.so",
    };
#endif
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

std::filesystem::path LibSm64Backend::findDefaultRom()
{
    if (!std::filesystem::exists("roms")) {
        return {};
    }
    for (const auto& entry : std::filesystem::directory_iterator("roms")) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto extension = entry.path().extension().string();
        if (extension == ".z64" || extension == ".n64" || extension == ".v64") {
            return entry.path();
        }
    }
    return {};
}

bool LibSm64Backend::loadLibrary(const std::filesystem::path& dllPath)
{
#if defined(_WIN32)
    library_ = LoadLibraryW(dllPath.wstring().c_str());
#else
    library_ = dlopen(dllPath.string().c_str(), RTLD_NOW);
#endif
    return library_ != nullptr;
}

bool LibSm64Backend::bindApi()
{
    api_ = new Api();
    api_->globalInit = reinterpret_cast<Api::GlobalInit>(loadSymbol(library_, "sm64_global_init"));
    api_->globalTerminate = reinterpret_cast<Api::GlobalTerminate>(loadSymbol(library_, "sm64_global_terminate"));
    api_->staticSurfacesLoad = reinterpret_cast<Api::StaticSurfacesLoad>(loadSymbol(library_, "sm64_static_surfaces_load"));
    api_->marioCreate = reinterpret_cast<Api::MarioCreate>(loadSymbol(library_, "sm64_mario_create"));
    api_->marioTick = reinterpret_cast<Api::MarioTick>(loadSymbol(library_, "sm64_mario_tick"));
    api_->marioDelete = reinterpret_cast<Api::MarioDelete>(loadSymbol(library_, "sm64_mario_delete"));

    return api_->globalInit && api_->globalTerminate && api_->staticSurfacesLoad
        && api_->marioCreate && api_->marioTick && api_->marioDelete;
}

void LibSm64Backend::unloadLibrary()
{
    if (!library_) {
        return;
    }
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(library_));
#else
    dlclose(library_);
#endif
    library_ = nullptr;
}

void LibSm64Backend::loadSurfaces(const collision::CollisionWorld& collisionWorld)
{
    std::vector<SM64Surface> surfaces;
    surfaces.reserve(collisionWorld.triangles().size());

    for (const auto& triangle : collisionWorld.triangles()) {
        SM64Surface surface {};
        surface.type = 0;
        surface.force = 0;
        surface.terrain = 0;
        const glm::vec3 vertices[] = { triangle.a, triangle.b, triangle.c };
        for (int i = 0; i < 3; ++i) {
            surface.vertices[i][0] = toSm64Coord(vertices[i].x);
            surface.vertices[i][1] = toSm64Coord(vertices[i].y);
            surface.vertices[i][2] = toSm64Coord(vertices[i].z);
        }
        surfaces.push_back(surface);
    }

    api_->staticSurfacesLoad(surfaces.data(), static_cast<std::uint32_t>(surfaces.size()));
}

} // namespace sm64ps::mario
