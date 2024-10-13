//
// Created by Ethan Lee on 7/24/24.
//

#ifndef JOSHENGINE_GAMEPHYSICSLIB_H
#define JOSHENGINE_GAMEPHYSICSLIB_H
#include "engine/engine.h"

[[nodiscard]] bool testPoint(glm::vec3 point, const JE::Transform& box);
[[nodiscard]] bool testSpheres(const glm::vec3& p1, const float& r1, const glm::vec3& p2, const float& r2);
[[nodiscard]] bool testSpheres(const JE::Transform& t1, const JE::Transform& t2);
[[nodiscard]] bool pointCollidesWithAnyBoxes(const vec3& point, const std::vector<JE::Transform>& worldBoxColliders);
[[nodiscard]] bool sphereCollidesWithAnySpheres(const JE::Transform& t1, const std::vector<JE::Transform>& colliders);

#endif //JOSHENGINE_GAMEPHYSICSLIB_H
