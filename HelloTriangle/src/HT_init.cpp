#include "HT.hpp"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include "memory/vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

void HelloTriangle::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

void HelloTriangle::initVulkan() {
    createInstance();
    createSurface();
    createDevice();
    createVMA();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createColorResources();
    createDepthResources();
    createTextureImage();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createCommandBuffer();
    createSyncObjects();
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT       severity,
                                                      vk::DebugUtilsMessageTypeFlagsEXT              type,
                                                      const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
                                                      void *                                         pUserData)
{
  std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

  return vk::False;
}

void HelloTriangle::createInstance() {
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName   = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION( 1, 0, 0),
        .pEngineName        = "No Engine", 
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = vk::ApiVersion14
    };

    // Get GLFW instance extensions
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(vk::EXTDebugUtilsExtensionName);

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    instance = vk::raii::Instance(context, createInfo);

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );

    vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
                                                                          .messageType     = messageTypeFlags,
                                                                          .pfnUserCallback = &debugCallback};
    debugMessenger = instance.createDebugUtilsMessengerEXT( debugUtilsMessengerCreateInfoEXT );
}


void HelloTriangle::createSurface() {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
}


void HelloTriangle::createDevice() {
    physicalDevice = instance.enumeratePhysicalDevices()[0];
    std::vector<const char*> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName,
        vk::KHRPushDescriptorExtensionName,
        // for VMA
        vk::KHRDedicatedAllocationExtensionName,
        vk::KHRBindMemory2ExtensionName,
        vk::EXTMemoryBudgetExtensionName,
        vk::KHRBufferDeviceAddressExtensionName
    };


    auto properties = physicalDevice.getProperties();

    std::cout << "Max UBO range: "
          << properties.limits.maxUniformBufferRange << '\n';

    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
        {
        // found a queue family that supports both graphics and present
        queueIndex = qfpIndex;
        break;
        }
    }
    if (queueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    // query for Vulkan 1.3 features
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    > featureChain = {
        {.features = {.sampleRateShading = true, .samplerAnisotropy = true}},
        {.shaderDrawParameters = true},
        {.bufferDeviceAddress = true},
        {.synchronization2 = true, .dynamicRendering = true},
        {.extendedDynamicState = true}
    };

    // create a Device
    float                     queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
    vk::DeviceCreateInfo      deviceCreateInfo{
        .pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data()
    };

    device = vk::raii::Device( physicalDevice, deviceCreateInfo );
    queue  = vk::raii::Queue(device, queueIndex, 0);
}

void HelloTriangle::createVMA() {
    VmaAllocatorCreateFlags flags = 
        VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
        VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT |
        VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
        VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VmaAllocatorCreateInfo allocatorCreateInfo {
        .flags = flags,
        .physicalDevice = *physicalDevice,
        .device = *device,
        .instance = *instance,
        .vulkanApiVersion = VK_API_VERSION_1_4,
    };

    allocator = ht::Allocator(allocatorCreateInfo);
}


void HelloTriangle::createSwapChain() {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    swapChainExtent = vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    uint32_t minImageCount = 3u;

    auto availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
    const auto formatIt = std::ranges::find_if(
        availableFormats,
        [](const auto &format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
    swapChainSurfaceFormat = formatIt[0];

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface          = *surface,
        .minImageCount    = minImageCount,
        .imageFormat      = swapChainSurfaceFormat.format,
        .imageColorSpace  = swapChainSurfaceFormat.colorSpace,
        .imageExtent      = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform     = surfaceCapabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = vk::PresentModeKHR::eImmediate,
        .clipped          = true
    };
    swapChain       = vk::raii::SwapchainKHR( device, swapChainCreateInfo );
    swapChainImages = swapChain.getImages();
}


void HelloTriangle::createImageViews(){
  assert(swapChainImageViews.empty());

  swapChainImageViews.reserve(swapChainImages.size());
  for ( auto &image: swapChainImages )
  {
    swapChainImageViews.emplace_back(createImageView(image, swapChainSurfaceFormat.format,  vk::ImageAspectFlagBits::eColor, 1));
  }
}


void HelloTriangle::createSyncObjects() {

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
        inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

vk::raii::CommandBuffer HelloTriangle::beginSingleTimeCommands(){
  vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
  vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

  vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
  commandBuffer.begin(beginInfo);

  return std::move(commandBuffer);
}

void HelloTriangle::endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer){
  commandBuffer.end();

  vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
  queue.submit(submitInfo, nullptr);
  queue.waitIdle();
}

vk::raii::ImageView HelloTriangle::createImageView(
    vk::Image const &image,
    vk::Format format,
    vk::ImageAspectFlags aspectFlags,
    uint32_t mipLevels
) {
    vk::ImageViewCreateInfo viewInfo{
        .image            = image,
        .viewType         = vk::ImageViewType::e2D,
        .format           = format,
        .subresourceRange = {.aspectMask = aspectFlags, .levelCount = mipLevels, .layerCount = 1}
    };

    return vk::raii::ImageView(device, viewInfo);
}


void HelloTriangle::createTextureImage()
{
    int            texWidth, texHeight, texChannels;
    stbi_uc       *pixels    = stbi_load("cat.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    std::cout << "Mip Levels = " << mipLevels << std::endl;

    vk::ImageCreateInfo imageInfo{
        .imageType   = vk::ImageType::e2D,
        .format      = vk::Format::eR8G8B8A8Srgb,
        .extent      = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
        .mipLevels   = mipLevels,
        .arrayLayers = 1,
        .samples     = vk::SampleCountFlagBits::e1,
        .tiling      = vk::ImageTiling::eOptimal,
        .usage       = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::ImageViewCreateInfo viewInfo {
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = mipLevels,
            .layerCount = 1
        }
    };

    newTextureImage = ht::Image(
        &allocator,
        device,
        imageInfo,
        viewInfo,
        {}
    );


    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();

    newTextureImage.init_layout(
        commandBuffer,        
        vk::ImageLayout::eGeneral,
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eCopy,
        vk::ImageAspectFlagBits::eColor
    );


    ht::Buffer _stagingBuffer = newTextureImage.copyMemoryStaged(
        pixels,
        imageSize,
        commandBuffer
    );

    stbi_image_free(pixels);

    newTextureImage.gen_mipmaps(commandBuffer);

    endSingleTimeCommands(std::move(commandBuffer));

}

void HelloTriangle::createTextureSampler() {
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo        samplerInfo{
        .magFilter        = vk::Filter::eLinear,
        .minFilter        = vk::Filter::eLinear,
        .mipmapMode       = vk::SamplerMipmapMode::eLinear,
        .addressModeU     = vk::SamplerAddressMode::eRepeat,
        .addressModeV     = vk::SamplerAddressMode::eRepeat,
        .addressModeW     = vk::SamplerAddressMode::eRepeat,
        .mipLodBias       = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
        .minLod           = 0.0f,
        .maxLod           = static_cast<float>(mipLevels - 1)
    };

    textureSampler = vk::raii::Sampler(device, samplerInfo);
}

void HelloTriangle::createDepthResources() {
    vk::ImageCreateInfo imageInfo{
        .imageType   = vk::ImageType::e2D,
        .format      = vk::Format::eD32Sfloat,
        .extent      = {swapChainExtent.width, swapChainExtent.height, 1},
        .mipLevels   = 1,
        .arrayLayers = 1,
        .samples     = msaaSamples,
        .tiling      = vk::ImageTiling::eOptimal,
        .usage       = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::ImageViewCreateInfo viewInfo{
        .viewType         = vk::ImageViewType::e2D,
        .format           = vk::Format::eD32Sfloat,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eDepth, .levelCount = 1, .layerCount = 1}
    };

    depthImage = ht::Image(&allocator, device, imageInfo, viewInfo, {});
}

void HelloTriangle::createColorResources() {
    vk::Format colorFormat = swapChainSurfaceFormat.format;

    vk::ImageCreateInfo imageInfo{
        .imageType   = vk::ImageType::e2D,
        .format      = colorFormat,
        .extent      = {swapChainExtent.width, swapChainExtent.height, 1},
        .mipLevels   = 1,
        .arrayLayers = 1,
        .samples     = msaaSamples,
        .tiling      = vk::ImageTiling::eOptimal,
        .usage       = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        .sharingMode = vk::SharingMode::eExclusive
    };

    vk::ImageViewCreateInfo viewInfo{
        .viewType         = vk::ImageViewType::e2D,
        .format           = colorFormat,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1}
    };

    colorImage = ht::Image(&allocator, device, imageInfo, viewInfo, {});
}