//
// Created by cdgira on 6/30/2023.
//
#include "first_app.hpp"

#include "keyboard_movement_controller.hpp"
#include "lve_camera.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "lve_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <iostream>
#include <array>
#include <chrono>

namespace lve {



    FirstApp::FirstApp() {
        // We need to add a pool for the textureImages.
        globalPool = LveDescriptorPool::Builder(lveDevice)
                .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * LveSwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * LveSwapChain::MAX_FRAMES_IN_FLIGHT) // This is for the texture maps.
                .build();

        loadGameObjects();
        // Texture Image loaded in first_app.hpp file.
    }

    FirstApp::~FirstApp() { }

    void FirstApp::run() {
        // The GlobalUbo is a fixed size so we can setup this up here then load in the data later.
        // So I need to load in the images first.
        std::vector<std::unique_ptr<LveBuffer>> uboBuffers(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i=0;i<uboBuffers.size();i++) {
            uboBuffers[i] = std::make_unique<LveBuffer>(
                    lveDevice,
                    sizeof(GlobalUbo),
                    1,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffers[i]->map();
        }

        auto globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,VK_SHADER_STAGE_ALL_GRAPHICS)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();
        // Need to see if anything needs to be done here for the texture maps.
        // Something isn't beting setup right for the Image Info, information is not getting freed correctly.
        std::vector<VkDescriptorSet> globalDescriptorSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i=0;i<globalDescriptorSets.size();i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            auto imageInfo = planetImage->descriptorImageInfo();
            auto imageInfo2 = sharkImage->descriptorImageInfo();
            auto imageInfo3 = shipImage->descriptorImageInfo();
            auto imageInfo4 = backgroundImage->descriptorImageInfo();
            auto imageInfo5 = monsterETImage->descriptorImageInfo();
            LveDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .writeImage(1, &imageInfo)
                .writeImage(2, &imageInfo2)
                .writeImage(3, &imageInfo3)
                .writeImage(4, &imageInfo4)
                .writeImage(5, &imageInfo5)
                .build(globalDescriptorSets[i]); // Should only build a set once.
        }

        SimpleRenderSystem simpleRenderSystem{lveDevice, lveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
        PointLightSystem pointLightSystem{lveDevice, lveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
        LveCamera camera{};
        camera.setViewTarget(glm::vec3(-1.f, -2.f, -2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = LveGameObject::createGameObject();
        viewerObject.transform.translation.z = -2.5f;
        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();


        glm::vec3 monsterStart = {0.0f, -1.0f, 16.0f};
        glm::vec3 shipStart = {1.f, 0.5f, -0.f};
        glm::vec3 planetStart {-0.0f, 1.5f, 10.f};
        glm::vec3 planetOriginalRotation;
        glm::vec3 monsterOriginalScale;

        float frameCount = 0.0f;

        while (!lveWindow.shouldClose()) {
            glfwPollEvents();

            glm::vec3 cameraPosition = camera.getCameraPos();
            glm::vec3 cameraTarget = camera.getCameraPos() + glm::vec3(camera.getView()[2]);

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ(lveWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = lveRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);
            if (auto commandBuffer = lveRenderer.beginFrame()) {
                int frameIndex = lveRenderer.getFrameIndex();
                FrameInfo frameInfo{frameIndex, frameTime, commandBuffer,camera, globalDescriptorSets[frameIndex], gameObjects};
                //update
                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
                ubo.inverseView = camera.getInverseView();
                pointLightSystem.update(frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                //render
                lveRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem.render(frameInfo); // Solid Objects
                pointLightSystem.render(frameInfo);  // Transparent Objects
                lveRenderer.endSwapChainRenderPass(commandBuffer);
                lveRenderer.endFrame();
            }

            //Monster Animation Process
            if ((glfwGetKey(lveWindow.getGLFWwindow(), GLFW_KEY_1) == GLFW_PRESS) && (!isAnimatingMonster)) {
                isAnimatingMonster = true;
            }

            if(isAnimatingMonster) {
                auto &monster = gameObjects.at(MONSTER_ID);
                isAnimatingMonster = monster.transform.update(frameTime);
                printf("%i\n",isAnimatingMonster);
            }

            //Ship Animation Process
            if (glfwGetKey(lveWindow.getGLFWwindow(), GLFW_KEY_2) == GLFW_PRESS) {
                isAnimatingShip = true;
            }

            if(isAnimatingShip) {
                auto &ship = gameObjects.at(SHIP_ID);
                isAnimatingShip = ship.transform.update(frameTime);
            }

            //Planet Animation Process
            if (glfwGetKey(lveWindow.getGLFWwindow(), GLFW_KEY_3) == GLFW_PRESS) {
                isAnimatingPlanet = true;
            }

            if(isAnimatingPlanet) {
                auto &planet = gameObjects.at(PLANET_ID);
                isAnimatingPlanet = planet.transform.update(frameTime);
            }
        }

        vkDeviceWaitIdle(lveDevice.device());
    }

    void FirstApp::loadGameObjects() {

        float animationDuration = 4.0f;

        std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "../models/shark_01.obj");

        lveModel = LveModel::createModelFromFile(lveDevice, "../models/Stylized_Planets.obj");
        auto planet = LveGameObject::createGameObject();
        planet.model = lveModel;
        planet.transform.translation = {-0.0f, 1.5f, 10.f};
        planet.transform.scale = {-2.f, -2.f, -2.f};
        planet.textureBinding = 1;
        PLANET_ID = planet.getId();
        AnimationSequence planetAnimation;

                AnimationKeyFrame planetStartFrame;
                planetStartFrame.translation = {-0.0f, 1.5f, 10.f}; // Your original position
                planetStartFrame.scale = glm::vec3(-2.f, -2.f, -2.f); // Initial scale
                planetStartFrame.rotation = glm::vec3(0); // Assuming no rotation
                planetStartFrame.timeStamp = 0.0f;

                AnimationKeyFrame planetEndFrame;
                planetEndFrame.translation = {-0.0f, 1.5f, 10.f}; // Ending position after 9 seconds
                planetEndFrame.scale = planetStartFrame.scale + glm::vec3(-0.5); // Adjust scale based on duration
                planetEndFrame.rotation = glm::vec3(-1, 0.0 ,0); // Assuming no rotation
                planetEndFrame.timeStamp = 3.9f;

                AnimationKeyFrame planetLastFrame;
                planetLastFrame.translation = {-0.0f, 1.5f, 10.f}; // Your original position
                planetLastFrame.scale = glm::vec3(-2.f, -2.f, -2.f); // Initial scale
                planetLastFrame.rotation = glm::vec3(0); // Assuming no rotation
                planetLastFrame.timeStamp = animationDuration;

                planetAnimation.keyFrames.push_back(planetStartFrame);
                planetAnimation.keyFrames.push_back(planetEndFrame);
                planetAnimation.keyFrames.push_back(planetLastFrame);
                planetAnimation.duration = planetLastFrame.timeStamp;


        planet.transform.animationSequence = planetAnimation;
        gameObjects.emplace(PLANET_ID,std::move(planet));

        //CHILD OBJECT TO PLANET
        auto childPlanet = LveGameObject::createGameObject();
        //childPlanet.setParent(planet); // Set the larger planet as the parent of the smaller planet

        // Set the child planet's local transformation relative to the parent
        childPlanet.transform.translation = {1.0f, 0.0f, 0.0f}; // Position relative to the parent planet
        childPlanet.transform.scale = {0.5f, 0.5f, 0.5f}; // Scale to make it smaller
        // ... other properties like model, texture, etc. ...



        //GIANT MONSTER MODEL
        lveModel = LveModel::createModelFromFile(lveDevice, "../models/Giant Monster Fish.OBJ");
        auto shark2 = LveGameObject::createGameObject();
        shark2.model = lveModel;
        glm::vec3 monsterStart = {0.0f, -1.0f, 16.0f};
        shark2.transform.translation = monsterStart;
        shark2.transform.scale = {-0.1f, -0.1f, -0.1f};
        shark2.textureBinding = 2;
        MONSTER_ID = shark2.getId();

        AnimationSequence monsterAnimation;

            AnimationKeyFrame monsterStartFrame;
            monsterStartFrame.translation = monsterStart; // Your original position
            monsterStartFrame.scale = glm::vec3(-0.1f, -0.1f, -0.1f); // Initial scale
            monsterStartFrame.rotation = glm::vec3(0); // Assuming no rotation
            monsterStartFrame.timeStamp = 0.0f;

            AnimationKeyFrame monsterEndFrame;
            monsterEndFrame.translation = monsterStart + glm::vec3(0,0,-1.0f); // Ending position after 9 seconds
            monsterEndFrame.scale = monsterStartFrame.scale + glm::vec3(-0.1); // Adjust scale based on duration
            monsterEndFrame.rotation = glm::vec3(0); // Assuming no rotation
            monsterEndFrame.timeStamp = 3.9f;

            AnimationKeyFrame monsterLastFrame;
            monsterLastFrame.translation = monsterStart; // Your original position
            monsterLastFrame.scale = glm::vec3(-0.1f, -0.1f, -0.1f); // Initial scale
            monsterLastFrame.rotation = glm::vec3(0); // Assuming no rotation
            monsterLastFrame.timeStamp = animationDuration;

        monsterAnimation.keyFrames.push_back(monsterStartFrame);
        monsterAnimation.keyFrames.push_back(monsterEndFrame);
        monsterAnimation.keyFrames.push_back(monsterLastFrame);
        monsterAnimation.duration = monsterLastFrame.timeStamp;

        shark2.transform.animationSequence = monsterAnimation;
        gameObjects.emplace(MONSTER_ID,std::move(shark2));

        //SPACE SHIP MODEL
        lveModel = LveModel::createModelFromFile(lveDevice, "../models/HeavyBattleship.obj");
        auto spaceShip = LveGameObject::createGameObject();
        glm::vec3 shipStart = {1.f, 0.5f, -0.f};
        spaceShip.model = lveModel;
        spaceShip.transform.translation = shipStart;
        spaceShip.transform.scale = {.00008f, -.00008f, -.00008f};
        spaceShip.textureBinding = 3;
        SHIP_ID = spaceShip.getId();

        AnimationSequence shipAnimation;
            AnimationKeyFrame shipStartFrame;
            shipStartFrame.translation = {1.f, 0.5f, -0.f}; // Your original position
            shipStartFrame.scale = glm::vec3(.00008f, -.00008f, -.00008f); // Initial scale
            shipStartFrame.rotation = glm::vec3(0); // Assuming no rotation
            shipStartFrame.timeStamp = 0.0f;

            AnimationKeyFrame shipEndFrame;
            shipEndFrame.translation = shipStart + glm::vec3(0, 0, 1.0f * 4.0f); // Ending position after 9 seconds
            shipEndFrame.scale = shipStartFrame.scale + glm::vec3(9.0f * 0.000005f); // Adjust scale based on duration
            shipEndFrame.rotation = glm::vec3(0); // Assuming no rotation
            shipEndFrame.timeStamp = 3.9f;

            AnimationKeyFrame shipLastFrame;
            shipLastFrame.translation = {1.f, 0.5f, -0.f}; // Your original position
            shipLastFrame.scale = glm::vec3(.00008f, -.00008f, -.00008f); // Initial scale
            shipLastFrame.rotation = glm::vec3(0); // Assuming no rotation
            shipLastFrame.timeStamp = 4.0f;

        shipAnimation.keyFrames.push_back(shipStartFrame);
        shipAnimation.keyFrames.push_back(shipEndFrame);
        shipAnimation.keyFrames.push_back(shipLastFrame);
        shipAnimation.duration = shipLastFrame.timeStamp;

        spaceShip.transform.animationSequence = shipAnimation;
        gameObjects.emplace(SHIP_ID,std::move(spaceShip));

        //BACKGROUND MODEL
        lveModel = LveModel::createModelFromFile(lveDevice, "../models/Background.obj");
        auto background = LveGameObject::createGameObject();
        background.model = lveModel;
        background.transform.translation = {30.f, 30.f, 15.f};
        background.transform.scale = {-30.f, -20.f, 0.0f};
        background.textureBinding = 4;
        gameObjects.emplace(background.getId(),std::move(background));

        std::vector<glm::vec3> lightColors{
            {1.f, .1f, .1f},
            {.1f, .1f, 1.f},
            {.1f, 1.f, .1f},
            {1.f, 1.f, .1f},
            {.1f, 1.f, 1.f},
            {1.f, 1.f, 1.f}  //
        };


        auto shipLight = LveGameObject::makePointLight(0.3f);
        shipLight.color = {1.0f, 1.0f, 1.0f};
        shipLight.transform.translation = {1.f, 0.5f, -0.f};
        gameObjects.emplace(shipLight.getId(), std::move(shipLight));

        auto shipLight2 = LveGameObject::makePointLight(0.015f);
        shipLight2.color = {0.0f, 0.0f, 1.0f};
        shipLight2.transform.translation = {1.f, 0.55f, -1.2f};
        gameObjects.emplace(shipLight2.getId(), std::move(shipLight2));

        auto thrusterlight = LveGameObject::makePointLight(0.016f);
        thrusterlight.color = {0.0f, 0.0f, 1.0f};
        thrusterlight.transform.translation = {1.f, 0.3f, -1.2f};
        gameObjects.emplace(thrusterlight.getId(), std::move(thrusterlight));

        auto planetLight = LveGameObject::makePointLight(10.f);
        planetLight.color = {1.0f, 0.0f, 0.0f};
        planetLight.transform.translation = {-0.0f, 1.5f, 10.f};
        gameObjects.emplace(planetLight.getId(), std::move(planetLight));

        auto ambience = LveGameObject::makePointLight(4.f);
        ambience.color = {1.0f, 1.0f, 1.0f};
        ambience.transform.translation = {5.0f, -5.f, 10.f};
        gameObjects.emplace(ambience.getId(), std::move(ambience));

        auto sharkLight = LveGameObject::makePointLight(4.f);
        sharkLight.color = {1.0f, 1.0f, 1.0f};
        sharkLight.transform.translation = {2.f, 1.4f, 9.5f};
        gameObjects.emplace(sharkLight.getId(), std::move(sharkLight));

        auto monsterLight = LveGameObject::makePointLight(10.f);
        monsterLight.color = {-1.0f, -0.0f, -15.0f};
        monsterLight.transform.translation = {1.0f, -1.5f, 11.0f};
        gameObjects.emplace(monsterLight.getId(), std::move(monsterLight));
    }



}

