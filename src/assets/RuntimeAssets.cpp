#include "assets/RuntimeAssets.h"

#include "util/Log.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

#include <glm/geometric.hpp>

namespace sm64ps::assets {

namespace {

bool startsWith(const std::string& value, const char* prefix)
{
    return value.rfind(prefix, 0) == 0;
}

glm::vec3 parseVec3(std::istringstream& stream)
{
    glm::vec3 value {};
    stream >> value.x >> value.y >> value.z;
    return value;
}

int parseObjIndex(const std::string& token, std::size_t element)
{
    std::size_t begin = 0;
    std::size_t currentElement = 0;
    while (currentElement < element && begin != std::string::npos) {
        begin = token.find('/', begin);
        if (begin != std::string::npos) {
            ++begin;
        }
        ++currentElement;
    }

    if (begin == std::string::npos || begin >= token.size()) {
        return 0;
    }

    const std::size_t end = token.find('/', begin);
    const std::string slice = token.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    if (slice.empty()) {
        return 0;
    }
    return std::stoi(slice);
}

} // namespace

bool RuntimeAssets::loadFromDirectory(const std::filesystem::path& directory)
{
    marioMesh_ = {};
    const std::filesystem::path marioObj = directory / "mario.obj";
    if (!std::filesystem::exists(marioObj)) {
        status_ = "No extracted/mario.obj found; using procedural local stand-in.";
        return false;
    }

    if (!loadObj(marioObj, marioMesh_)) {
        status_ = "Failed to load extracted/mario.obj; using procedural local stand-in.";
        return false;
    }

    status_ = "Loaded local extracted Mario mesh: " + marioObj.string();
    util::logInfo(status_);
    return true;
}

bool RuntimeAssets::loadObj(const std::filesystem::path& path, Mesh& mesh)
{
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::string line;

    mesh.name = path.filename().string();
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream stream(line);
        std::string tag;
        stream >> tag;

        if (tag == "v") {
            positions.push_back(parseVec3(stream));
        } else if (tag == "vn") {
            normals.push_back(glm::normalize(parseVec3(stream)));
        } else if (tag == "f") {
            std::vector<std::string> tokens;
            std::string token;
            while (stream >> token) {
                tokens.push_back(token);
            }

            if (tokens.size() < 3) {
                continue;
            }

            for (std::size_t i = 1; i + 1 < tokens.size(); ++i) {
                const std::string triTokens[] = { tokens[0], tokens[i], tokens[i + 1] };
                glm::vec3 fallbackNormal(0.0f, 1.0f, 0.0f);
                if (positions.size() >= 3) {
                    const int ia = parseObjIndex(triTokens[0], 0);
                    const int ib = parseObjIndex(triTokens[1], 0);
                    const int ic = parseObjIndex(triTokens[2], 0);
                    if (ia > 0 && ib > 0 && ic > 0
                        && static_cast<std::size_t>(ia) <= positions.size()
                        && static_cast<std::size_t>(ib) <= positions.size()
                        && static_cast<std::size_t>(ic) <= positions.size()) {
                        fallbackNormal = glm::normalize(glm::cross(
                            positions[static_cast<std::size_t>(ib - 1)] - positions[static_cast<std::size_t>(ia - 1)],
                            positions[static_cast<std::size_t>(ic - 1)] - positions[static_cast<std::size_t>(ia - 1)]));
                    }
                }

                for (const std::string& triToken : triTokens) {
                    const int positionIndex = parseObjIndex(triToken, 0);
                    const int normalIndex = parseObjIndex(triToken, 2);
                    if (positionIndex <= 0 || static_cast<std::size_t>(positionIndex) > positions.size()) {
                        continue;
                    }
                    const glm::vec3 normal = normalIndex > 0 && static_cast<std::size_t>(normalIndex) <= normals.size()
                        ? normals[static_cast<std::size_t>(normalIndex - 1)]
                        : fallbackNormal;
                    mesh.vertices.push_back({ positions[static_cast<std::size_t>(positionIndex - 1)], normal });
                }
            }
        }
    }

    return !mesh.vertices.empty();
}

} // namespace sm64ps::assets

