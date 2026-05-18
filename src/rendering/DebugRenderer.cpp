#include "rendering/DebugRenderer.h"

namespace sm64ps::rendering {

void DebugRenderer::clear()
{
    lines_.clear();
}

void DebugRenderer::line(glm::vec3 a, glm::vec3 b, glm::vec3 color)
{
    lines_.push_back(DebugLine { a, b, color });
}

void DebugRenderer::vector(glm::vec3 origin, glm::vec3 value, glm::vec3 color, float scale)
{
    line(origin, origin + value * scale, color);
}

void DebugRenderer::normal(glm::vec3 point, glm::vec3 normalValue, glm::vec3 color, float scale)
{
    line(point, point + normalValue * scale, color);
}

} // namespace sm64ps::rendering

