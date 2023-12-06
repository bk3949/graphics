//
// Created by cdgira on 7/13/2023.
//

#ifndef VULKANTEST_LVE_GAME_OBJECT_HPP
#define VULKANTEST_LVE_GAME_OBJECT_HPP

#include "lve_model.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>

namespace lve {

    struct AnimationKeyFrame {
        glm::vec3 translation;
        glm::vec3 rotation;
        glm::vec3 scale;
        float timeStamp;
    };

    // Animation Sequence Structure
    struct AnimationSequence {
        std::vector<AnimationKeyFrame> keyFrames;
        float duration;
    };

    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::vec3 rotation{0.0f};

        AnimationSequence animationSequence;
        float currentTime = 0.0f;  // Current time since the animation started


        // Need to go over base form of each in class.
        // Need to show standard rotation matrix found in most books.
        glm::mat4 mat4() const;
        glm::mat4 normalMatrix();
        bool update(float deltaTime);  // New method to update based on animations
    };

    struct PointLightComponent {
        float lightIntensity = 1.0f;
    };

    class LveGameObject : public std::enable_shared_from_this<LveGameObject>{
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, LveGameObject>;

        static LveGameObject createGameObject() {
            static id_t currentId = 0;
            return LveGameObject{currentId++};
        }

        std::shared_ptr<LveGameObject> parent;  // Pointer to the parent object
        std::vector<std::shared_ptr<LveGameObject>> children;  // List of child objects

        // Method to get the world transformation
        glm::mat4 getWorldTransform() const {
            glm::mat4 parentTransform = parent ? parent->getWorldTransform() : glm::mat4(1.0f);
            return parentTransform * transform.mat4();
        }

        // Method to set the parent
        void setParent(std::shared_ptr<LveGameObject> newParent) {
            parent = newParent;
            if (newParent) {
                newParent->children.push_back(shared_from_this());
            }
        }

        static LveGameObject makePointLight(float intensity = 10.0f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f));

        LveGameObject(const LveGameObject&) = delete;
        LveGameObject &operator=(const LveGameObject&) = delete;
        LveGameObject(LveGameObject&&) = default;
        LveGameObject &operator=(LveGameObject&&) = default;

        id_t getId() const { return id; }


        glm::vec3 color{};
        TransformComponent transform{};
        int32_t textureBinding = -1;

        // Optional components
        std::shared_ptr<LveModel> model{};
        std::unique_ptr<PointLightComponent> pointLight = nullptr;

    private:
        LveGameObject(id_t id) : id(id) {}
        id_t id;
    };
}

#endif //VULKANTEST_LVE_GAME_OBJECT_HPP
