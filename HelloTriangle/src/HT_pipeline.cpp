#include "HT.hpp"
#include "Vertex.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include <glm/ext/matrix_transform.hpp>



#include <cstdint>
#include <glm/ext/matrix_clip_space.hpp>
#include <iostream>
#include <fstream>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const uint INSTANCE_COUNT = 100;

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}



void HelloTriangle::createGraphicsPipeline() {
    auto code = readFile("shaders/slang.spv");
    vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    vk::raii::ShaderModule shaderModule{ device, createInfo };

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain" 
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Allocate dynamical viewport & scissor
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport, vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // Input data
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    // assemble topology as triangle list
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};

    // Viewport & scissor 
    vk::Viewport viewport{
        0.0f, 0.0f,
        static_cast<float>(swapChainExtent.width),
        static_cast<float>(swapChainExtent.height),
        0.0f, 1.0f
    };

    vk::Rect2D scissor{vk::Offset2D{ 0, 0 }, swapChainExtent};
    
    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // default rasterization
    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f
    };

    // Multisampling turned on!
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = msaaSamples,
        .sampleShadingEnable = vk::True,
        .minSampleShading = 0.95f
    };

    // Color blending (alpha)
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable         = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp        = vk::BlendOp::eAdd,
        .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment
    };

    // pushconstants
    vk::PushConstantRange pushRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset     = 0,
        .size       = sizeof(PushData)
    };


    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushRange
    };
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainSurfaceFormat.format
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable       = vk::True,
        .depthWriteEnable      = vk::True,
        .depthCompareOp        = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable     = vk::False};

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {
            .stageCount          = 2,
            .pStages             = shaderStages,
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicState,
            .layout              = pipelineLayout,
            .renderPass          = nullptr
        },
        {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainSurfaceFormat.format,
            .depthAttachmentFormat = vk::Format::eD32Sfloat,
        }
    };

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

void HelloTriangle::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueIndex
    };

    commandPool = vk::raii::CommandPool(device, poolInfo);
}

void HelloTriangle::createCommandBuffer() {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void HelloTriangle::recordCommandBuffer(uint32_t imageIndex) {
    auto &commandBuffer = commandBuffers[frameIndex];
    commandBuffer.begin({});

    // Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
    transition_image_layout(
        &swapChainImages[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        {},                                                        
        vk::AccessFlagBits2::eColorAttachmentWrite,                
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor  
    );

    transition_image_layout(
        &*depthImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth
    );

    transition_image_layout(
        &*colorImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eGeneral,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
    );


    vk::ClearValue  clearColor = vk::ClearColorValue(0.1f, 0.1f, 0.1f, 1.0f);
    vk::ClearValue  clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = colorImageView,
        .imageLayout = vk::ImageLayout::eGeneral,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = swapChainImageViews[imageIndex],
        .resolveImageLayout = vk::ImageLayout::eGeneral,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eDontCare,
        .clearValue  = clearColor
    };

    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView   = depthImageView,
        .imageLayout = vk::ImageLayout::eGeneral,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eDontCare,
        .clearValue  = clearDepth
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea           = {.offset = {0, 0}, .extent = swapChainExtent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo,
        .pDepthAttachment     = &depthAttachmentInfo
    };

    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    commandBuffer.setViewport(0,
        vk::Viewport(0.0f,
                static_cast<float>(swapChainExtent.height),
                static_cast<float>(swapChainExtent.width),
                -static_cast<float>(swapChainExtent.height), 0.0f, 1.0f)
    );
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));

    commandBuffer.bindVertexBuffers(0, newVertexBuffer.handle(), {0});
    commandBuffer.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);

    glm::mat4 view = glm::lookAt(eye_pos, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height),
        0.1f, 100.0f);

    PushData pc{ .viewproj = proj * view, .instances = uniformBufferAddresses[frameIndex] };

    commandBuffer.pushConstants<PushData>(
        *pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        0,
        pc
    );

    vk::DescriptorImageInfo imageInfo {
        .sampler = *textureSampler,
        .imageView = *textureImageView,
        .imageLayout = vk::ImageLayout::eGeneral
    };

    vk::WriteDescriptorSet write{
        .dstBinding       = 1,
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo       = &imageInfo
    };


    commandBuffer.pushDescriptorSet(
        vk::PipelineBindPoint::eGraphics,
        *pipelineLayout,
        0,
        write
    );


    commandBuffer.drawIndexed(
        static_cast<uint32_t>(indices.size()),
        INSTANCE_COUNT,
        0,
        0,
        0
    );

    commandBuffer.endRendering();

    // After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
    transition_image_layout(
        &swapChainImages[imageIndex],
        vk::ImageLayout::eGeneral,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
        {},                                                     // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
        vk::ImageAspectFlagBits::eColor
    );

    commandBuffer.end();
}

void HelloTriangle::transition_image_layout(
    const vk::Image          *image,
    vk::ImageLayout         old_layout,
    vk::ImageLayout         new_layout,
    vk::AccessFlags2        src_access_mask,
    vk::AccessFlags2        dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::ImageAspectFlags image_aspect_flags
){
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = src_stage_mask,
        .srcAccessMask       = src_access_mask,
        .dstStageMask        = dst_stage_mask,
        .dstAccessMask       = dst_access_mask,
        .oldLayout           = old_layout,
        .newLayout           = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = *image,
        .subresourceRange    = {
                .aspectMask     = image_aspect_flags,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1}};
    vk::DependencyInfo dependency_info = {
        .dependencyFlags         = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier};
    commandBuffers[frameIndex].pipelineBarrier2(dependency_info);
}


uint32_t HelloTriangle::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties){
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        auto flags = memProperties.memoryTypes[i].propertyFlags;
        if ((typeFilter & (1 << i)) && (flags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> HelloTriangle::createBuffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties
){
    vk::BufferCreateInfo   bufferInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
    vk::raii::Buffer       buffer          = vk::raii::Buffer(device, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateFlagsInfo flagsInfo{
        .flags = vk::MemoryAllocateFlagBits::eDeviceAddress};

    vk::MemoryAllocateInfo allocInfo{
        .pNext = (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) ? &flagsInfo : nullptr,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)};

    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
    return {std::move(buffer), std::move(bufferMemory)};
}


void HelloTriangle::createVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VmaAllocationCreateInfo allocCreateInfo;
    newVertexBuffer = ht::Buffer(
        allocator,
        bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer,
        {.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, .usage = VMA_MEMORY_USAGE_AUTO}
    );

    newVertexBuffer.copyMemory(vertices.data(), bufferSize);
}

void HelloTriangle::createIndexBuffer() {
		vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		auto [stagingBuffer, stagingBufferMemory] =
		    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void *data = stagingBufferMemory.mapMemory(0, bufferSize);
		memcpy(data, indices.data(), (size_t) bufferSize);
		stagingBufferMemory.unmapMemory();

		std::tie(indexBuffer, indexBufferMemory) =
		    createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void HelloTriangle::copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size)
{
  vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
  commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{.size = size});
  endSingleTimeCommands(std::move(commandCopyBuffer));
}

void HelloTriangle::createDescriptorSetLayout() {
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings{{
        {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        }
    }};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void HelloTriangle::createUniformBuffers(){
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DeviceSize bufferSize = sizeof(InstanceData) * INSTANCE_COUNT;
        auto [buffer, bufferMem]  = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );


        uniformBufferAddresses[i] = device.getBufferAddress({.buffer = *buffer});
        uniformBuffers.emplace_back(std::move(buffer));
        uniformBuffersMemory.emplace_back(std::move(bufferMem));
        uniformBuffersMapped.emplace_back( uniformBuffersMemory.back().mapMemory(0, bufferSize));

    }
}

void HelloTriangle::updateUniformBuffer(uint32_t currentImage)
{
    //static auto startTime = std::chrono::high_resolution_clock::now();

    //auto currentTime = std::chrono::high_resolution_clock::now();
    //float time       = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


    std::vector<InstanceData> instances(INSTANCE_COUNT);

    for(int i = 0; i < INSTANCE_COUNT; ++i){
        float float_i = (float) i;
        float theta = 2 * glm::pi<float>() * float_i / ( (float) INSTANCE_COUNT);

        instances[i] = {
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(
                    4.0f * glm::pow(glm::sin(theta), 3.0f),
                    0.25f * ( 13.0f * glm::cos(theta) - 5.0f * glm::cos(2*theta) - 2.0f * glm::cos(3 * theta) - glm::cos(theta)),
                    1.0f - 0.001f * float_i
                )
            )
        };
    }

    memcpy(uniformBuffersMapped[currentImage], instances.data(), sizeof(InstanceData) * INSTANCE_COUNT);
}


void HelloTriangle::transitionImageLayout(
    vk::raii::CommandBuffer &commandBuffer,
    const vk::raii::Image &image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags srcAccessMask,
    vk::AccessFlags dstAccessMask,
    vk::PipelineStageFlags srcStage,
    vk::PipelineStageFlags dstStage
){
    vk::ImageMemoryBarrier barrier{
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image               = image,
        .subresourceRange    = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = mipLevels,
            .layerCount = 1
        }
    };

    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;

    commandBuffer.pipelineBarrier(
        srcStage, dstStage, {}, {}, {}, barrier
    );
}


void HelloTriangle::copyBufferToImage(
    vk::raii::CommandBuffer &commandBuffer,
    const vk::raii::Buffer &buffer,
    vk::raii::Image &image,
    uint32_t width, uint32_t height
){
    vk::BufferImageCopy region{
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
        .imageOffset       = {0, 0, 0},
        .imageExtent       = {width, height, 1}
    };

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eGeneral, region);
}
