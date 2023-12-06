//
// Created by cdgira on 7/31/2023.
//
#include "lve_game_object.hpp"


namespace lve {

    glm::mat4 TransformComponent::mat4() const {
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.y);
        const float s2 = glm::sin(rotation.y);
        const float c1 = glm::cos(rotation.x);
        const float s1 = glm::sin(rotation.x);
        return glm::mat4{
                {
                        scale.x * (c1 * c3 + s1 * s2 * s3),
                        scale.x * (c2 * s3),
                        scale.x * (c1 * s2 * s3 - c3 * s1),
                        0.0f},
                {
                        scale.y * (c3 * s1 * s2 - c1 * s3),
                        scale.y * (c2 * c3),
                        scale.y * (c1 * c3 * s2 + s1 * s3),
                        0.0f},
                {
                        scale.z * (c2 * s1),
                        scale.z * (-s2),
                        scale.z * (c1 * c2),
                        0.0f},
                {translation.x, translation.y, translation.z, 1.0f}};
    }

    glm::mat4 TransformComponent::normalMatrix(){
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.y);
        const float s2 = glm::sin(rotation.y);
        const float c1 = glm::cos(rotation.x);
        const float s1 = glm::sin(rotation.x);
        const glm::vec3 invScale = 1.0f / scale;
        return glm::mat4{
                {
                        invScale.x * (c1 * c3 + s1 * s2 * s3),
                        invScale.x * (c2 * s3),
                        invScale.x * (c1 * s2 * s3 - c3 * s1),
                        0.0f},
                {
                        invScale.y * (c3 * s1 * s2 - c1 * s3),
                        invScale.y * (c2 * c3),
                        invScale.y * (c1 * c3 * s2 + s1 * s3),
                        0.0f},
                {
                        invScale.z * (c2 * s1),
                        invScale.z * (-s2),
                        invScale.z * (c1 * c2),
                        0.0f},
                {0.0f, 0.0f, 0.0f, 1.0f}};
        }

    LveGameObject LveGameObject::makePointLight(float intensity, float radius, glm::vec3 color) {
        LveGameObject gameObj = LveGameObject::createGameObject();
        gameObj.color = color;
        gameObj.transform.scale.x = radius;
        gameObj.pointLight = std::make_unique<PointLightComponent>();
        gameObj.pointLight->lightIntensity = intensity;
        return gameObj;
    }

    bool TransformComponent::update(float deltaTime) {
        currentTime += deltaTime;
        //printf("Test 1\n");
        // If animation is over, reset
        if (currentTime > animationSequence.duration) {
            currentTime = 0.0f;
            printf("Here\n");
            return false;
        }
        //printf("Test 2\n");
        AnimationKeyFrame *prevFrame = nullptr;
        AnimationKeyFrame *nextFrame = nullptr;
        //printf("Test 3\n");
        //printf("Starting loop with keyFrames size: %zu\n", animationSequence.keyFrames.size());
        if (animationSequence.keyFrames.size() < 2) {
            // Handle this case or simply return if there's nothing to be done
            printf("stopped early");
            return false;
        }
        for (size_t i = 0; i < animationSequence.keyFrames.size() - 1; i++) {
            //printf("Test 3.5\n");
            //printf("Checking timestamp for index %zu\n", i);
            if (animationSequence.keyFrames[i].timeStamp <= currentTime &&
                animationSequence.keyFrames[i + 1].timeStamp > currentTime) {
                //printf("Test 3.75\n");
                prevFrame = &animationSequence.keyFrames[i];
                nextFrame = &animationSequence.keyFrames[i + 1];
                //printf("Test 4\n");
                break;
            }
        }
        if (prevFrame == nextFrame)
        {
            printf("Same frame");
        }

        if (prevFrame && nextFrame) {
            float alpha = (currentTime - prevFrame->timeStamp) / (nextFrame->timeStamp - prevFrame->timeStamp);
            translation = glm::mix(prevFrame->translation, nextFrame->translation, alpha);
            rotation = glm::mix(prevFrame->rotation, nextFrame->rotation, alpha);
            scale = glm::mix(prevFrame->scale, nextFrame->scale, alpha);
        } else {
            // Handle the case where suitable keyframes were not found.
            // Maybe set default values, or handle the error gracefully.
            printf("Error: suitable keyframes not found\n");
        }

        return true;

    }
}