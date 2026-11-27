#pragma once

#include "vulkan/vulkan.hpp"
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Vertex.hpp"
#include "memory/HT_memory.hpp"


constexpr uint32_t WIDTH = 1200; // 1200
constexpr uint32_t HEIGHT = 1000; // 1000
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class HelloTriangle {
public:
    void run();

private:
    // Initialization
    void initWindow();
    void initVulkan();

    void createInstance();
    void createSurface();
    void createDevice();
    void createVMA();
    void createSwapChain();
    void createImageViews();
    void createSyncObjects();

    // Pipeline
    void createDescriptorSetLayout();
    void createUniformBuffers();
    void createGraphicsPipeline();
    void createVertexBuffer();
    void createIndexBuffer();
    void createCommandPool();
    void createCommandBuffer();
    void createColorResources();
    void recordCommandBuffer(
        uint32_t                imageIndex
    );
    void transition_image_layout(
        const vk::Image         *image,
        vk::ImageLayout         old_layout,
        vk::ImageLayout         new_layout,
        vk::AccessFlags2        src_access_mask,
        vk::AccessFlags2        dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags
    );
    uint32_t findMemoryType(uint32_t, vk::MemoryPropertyFlags);
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize, vk::BufferUsageFlags, vk::MemoryPropertyFlags);
    void copyBuffer(vk::raii::Buffer&, vk::raii::Buffer&, vk::DeviceSize);
    void updateUniformBuffer(uint32_t);
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createDepthResources();

    // helper functions
    vk::raii::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::raii::CommandBuffer&&);
    void transitionImageLayout(vk::raii::CommandBuffer&, const vk::raii::Image&,
        vk::ImageLayout, vk::ImageLayout,
        vk::AccessFlags, vk::AccessFlags,
        vk::PipelineStageFlags, vk::PipelineStageFlags
    );
    void copyBufferToImage(vk::raii::CommandBuffer&, const vk::raii::Buffer&, vk::raii::Image&, uint32_t, uint32_t);
    vk::raii::ImageView createImageView(vk::Image const&, vk::Format, vk::ImageAspectFlags, uint32_t);

    // Main loop
    void mainLoop();
    void drawFrame();

    // Cleanup
    void cleanup();

    GLFWwindow *window = nullptr;

    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    vk::raii::Queue queue = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    uint32_t queueIndex = ~0;

    ht::Allocator allocator;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;
    vk::SurfaceFormatKHR   swapChainSurfaceFormat;
    vk::Extent2D           swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;

    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;

    vk::raii::CommandPool commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence>     inFlightFences;

    uint32_t frameIndex = 0;

    ht::Buffer  vertexBuffer;
    ht::Buffer  indexBuffer;

    std::array<ht::Buffer,
        MAX_FRAMES_IN_FLIGHT>           uniformBuffers;
    std::array<vk::DeviceAddress,
        MAX_FRAMES_IN_FLIGHT>           uniformBufferAddresses;

    
    ht::Image depthImage;
    ht::Image colorImage;

    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e4;


    const std::vector<Vertex> vertices = {
        // Front (+Z)
        {{-0.5f, -0.5f,  0.5f}, {0,0}},
        {{ 0.5f, -0.5f,  0.5f}, {1,0}},
        {{ 0.5f,  0.5f,  0.5f}, {1,1}},
        {{-0.5f,  0.5f,  0.5f},{0,1}},

        // Back (-Z)
        {{ 0.5f, -0.5f, -0.5f}, {0,0}},
        {{-0.5f, -0.5f, -0.5f}, {1,0}},
        {{-0.5f,  0.5f, -0.5f}, {1,1}},
        {{ 0.5f,  0.5f, -0.5f}, {0,1}},

        // Right (+X)
        {{ 0.5f, -0.5f,  0.5f}, {0,0}},
        {{ 0.5f, -0.5f, -0.5f}, {1,0}},
        {{ 0.5f,  0.5f, -0.5f}, {1,1}},
        {{ 0.5f,  0.5f,  0.5f}, {0,1}},

        // Left (-X)
        {{-0.5f, -0.5f, -0.5f}, {0,0}},
        {{-0.5f, -0.5f,  0.5f}, {1,0}},
        {{-0.5f,  0.5f,  0.5f}, {1,1}},
        {{-0.5f,  0.5f, -0.5f}, {0,1}},

        // Top (+Y)
        {{-0.5f,  0.5f,  0.5f}, {0,0}},
        {{ 0.5f,  0.5f,  0.5f}, {1,0}},
        {{ 0.5f,  0.5f, -0.5f}, {1,1}},
        {{-0.5f,  0.5f, -0.5f}, {0,1}},

        // Bottom (-Y)
        {{-0.5f, -0.5f, -0.5f}, {0,0}},
        {{ 0.5f, -0.5f, -0.5f}, {1,0}},
        {{ 0.5f, -0.5f,  0.5f}, {1,1}},
        {{-0.5f, -0.5f,  0.5f}, {0,1}},
    };

    const std::vector<uint16_t> indices = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9, 10, 10, 11,  8,  // Right
        12, 13, 14, 14, 15, 12,  // Left
        16, 17, 18, 18, 19, 16,  // Top
        20, 21, 22, 22, 23, 20   // Bottom
    };

    glm::vec3 eye_pos = glm::vec3(2.0f);
    float rot = 4.71238898f;
    float dist = 4.5f;
    float height = 2.0f;

    uint32_t               mipLevels          = 0;

    ht::Image              newTextureImage;

    vk::raii::Image        textureImage       = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;

    vk::raii::ImageView textureImageView = nullptr;
    vk::raii::Sampler      textureSampler     = nullptr;
};