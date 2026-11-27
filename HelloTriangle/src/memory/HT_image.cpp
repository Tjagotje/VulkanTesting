#include "vk_mem_alloc.h"
#include "HT_memory.hpp"
#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan_raii.hpp>

#include <iostream>

namespace ht {

Image::Image(
    ht::Allocator* allocator,
    const vk::raii::Device& device,
    vk::ImageCreateInfo imageInfo,
    vk::ImageViewCreateInfo viewInfo,
    VmaAllocationCreateInfo allocInfo
) : mAllocator(allocator) {
    mAllocation = VmaAllocation();
    mImg = vk::Image();

    VkImage raw;
    VmaAllocationInfo info;

    if (allocInfo.usage == 0)
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult result = vmaCreateImage(
        allocator->handle(),
        imageInfo,
        &allocInfo,
        &raw,
        &mAllocation,
        &info
    ); 
    if (result != VK_SUCCESS) {
        throw std::runtime_error("vmaCreateBuffer failed!");
    }

    mImg = raw;
    mMapped = info.pMappedData;

    viewInfo.image = mImg;
    mExtent = imageInfo.extent;

    mImgView = vk::raii::ImageView(device, viewInfo);
};

void Image::swap(Image& o) noexcept {
    std::swap(mAllocator, o.mAllocator);
    std::swap(mAllocation, o.mAllocation);
    std::swap(mImg, o.mImg);
    std::swap(mImgView, o.mImgView);
    std::swap(mMapped, o.mMapped);
}

void Image::transition_layout(
        vk::raii::CommandBuffer& commandBuffer,
        vk::ImageLayout          old_layout,
        vk::ImageLayout          new_layout,
        vk::AccessFlags2         src_access_mask,
        vk::AccessFlags2         dst_access_mask,
        vk::PipelineStageFlags2  src_stage_mask,
        vk::PipelineStageFlags2  dst_stage_mask,
        vk::ImageAspectFlags     aspect_flags
) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = src_stage_mask,
        .srcAccessMask       = src_access_mask,
        .dstStageMask        = dst_stage_mask,
        .dstAccessMask       = dst_access_mask,
        .oldLayout           = old_layout,
        .newLayout           = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = mImg,
        .subresourceRange    = {
                .aspectMask     = aspect_flags,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1}
    };

    vk::DependencyInfo dependency_info = {
        .dependencyFlags         = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier};

    commandBuffer.pipelineBarrier2(dependency_info);
}

void Image::copyMemoryStaged(
    const void *data,
    vk::DeviceSize size,
    const vk::raii::CommandPool& pool,
    const vk::raii::Device& device,
    const vk::raii::Queue& queue
){
    Buffer stagingBuffer = {
        mAllocator,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage= VMA_MEMORY_USAGE_AUTO
        }
    };

    stagingBuffer.copyMemory(data, size);

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    commandBuffer.begin(beginInfo);

    vk::BufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
        .imageExtent = mExtent
    };

    commandBuffer.copyBufferToImage(
        stagingBuffer.handle(),
        mImg,
        vk::ImageLayout::eGeneral,
        region
    );

    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();

    std::cout << "Done copying staged image!!" << std::endl;
}

void Image::destroy() {
    if (mImg) {
        vmaDestroyImage(mAllocator->handle(), mImg, mAllocation);
        mImgView = nullptr;
        mImg = nullptr;
    }
}

}