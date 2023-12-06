//
// Created by cdgira on 6/30/2023.
//

#ifndef VULKANTEST_FIRST_APP_HPP
#define VULKANTEST_FIRST_APP_HPP

#include "lve_window.hpp"
#include "lve_game_object.hpp"
#include "lve_device.hpp"
#include "lve_renderer.hpp"
#include "lve_descriptors.hpp"
#include "lve_image.hpp"


#include <memory>
#include <vector>

namespace lve {
    class FirstApp {

    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        FirstApp();
        ~FirstApp();

        FirstApp(const FirstApp&) = delete;
        FirstApp &operator=(const FirstApp&) = delete;

        void run();

    private:
        bool isAnimatingMonster = false;
        bool isAnimatingPlanet = false;
        bool isAnimatingShip = false;
        int MONSTER_ID, PLANET_ID, SHIP_ID;
        void loadGameObjects();

        LveWindow lveWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
        LveDevice lveDevice{lveWindow};
        std::shared_ptr<LveImage> planetImage = LveImage::createImageFromFile(lveDevice, "../textures/Saturn2.png");
        std::shared_ptr<LveImage> sharkImage = LveImage::createImageFromFile(lveDevice, "../textures/Monster_Color.jpg");
        std::shared_ptr<LveImage> shipImage = LveImage::createImageFromFile(lveDevice, "../textures/Metal.png");
        std::shared_ptr<LveImage> backgroundImage = LveImage::createImageFromFile(lveDevice, "../textures/background.png");
        std::shared_ptr<LveImage> monsterETImage = LveImage::createImageFromFile(lveDevice, "../textures/Eyes_and_teeth_Color.jpg");
        // Here we need to setup the TextureMapping.
        //createTextureImage();
        //createTextureImageView();
        // Here is where they create the VertexBuffer, IndexBuffer, and UniformBuffer.
        LveRenderer lveRenderer{lveWindow, lveDevice};

        //note: Order of declaration is important.
        std::unique_ptr<LveDescriptorPool> globalPool{};
        LveGameObject::Map gameObjects;
    };
}

#endif //VULKANTEST_FIRST_APP_HPP
