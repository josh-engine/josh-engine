//
// Created by Ethan Lee on 7/24/24.
//

#include "gamephysicslib.h"

[[nodiscard]] bool testPoint(glm::vec3 point, const Transform& box){
    point -= box.position;
    point =  vec4(point, 1) * box.getRotateMatrix();
    point /= box.scale;
    const float boxSize = 1.0f;
    return (point.x < boxSize && point.x > -boxSize) && (point.y < boxSize && point.y > -boxSize) && (point.z < boxSize && point.z > -boxSize);
}

[[nodiscard]] bool testSpheres(const glm::vec3& p1, const float& r1, const glm::vec3& p2, const float& r2) {
    return r1 + r2 > distance(p1, p2);
}

[[nodiscard]] bool testSpheres(const Transform& t1, const Transform& t2) {
    return t1.scale.x + t2.scale.x > distance(t1.position, t2.position);
}

[[nodiscard]] bool pointCollidesWithAnyBoxes(const vec3& point, const std::vector<Transform>& colliders) {
    return std::ranges::any_of(colliders.cbegin(), colliders.cend(), [&point](const Transform& box) {return testPoint(point, box);});
}

[[nodiscard]] bool sphereCollidesWithAnySpheres(const Transform& t1, const std::vector<Transform>& colliders) {
    return std::ranges::any_of(colliders.cbegin(), colliders.cend(), [&t1](const Transform& sphere) {return testSpheres(t1, sphere);});
}