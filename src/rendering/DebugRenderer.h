#pragma once

#include <vector>

#include <glm/vec3.hpp>

namespace sm64ps::rendering {

struct DebugLine {
    glm::vec3 a { 0.0f };
    glm::vec3 b { 0.0f };
    glm::vec3 color { 1.0f };
};

class DebugRenderer {
public:
    void clear();
    void line(glm::vec3 a, glm::vec3 b, glm::vec3 color);
    void vector(glm::vec3 origin, glm::vec3 value, glm::vec3 color, float scale = 1.0f);
    void normal(glm::vec3 point, glm::vec3 normal, glm::vec3 color, float scale = 8.0f);

    const std::vector<DebugLine>& lines() const { return lines_; }

private:
    std::vector<DebugLine> lines_;
};

} // namespace sm64ps::rendering

