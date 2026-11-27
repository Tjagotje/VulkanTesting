#include "HT_memory.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>

namespace ht{


Buffer::Buffer(
    ht::Allocator* allocator,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    VmaAllocationCreateInfo allocInfo
) : mAllocator(allocator), mSize(size) {
    mAllocation = VmaAllocation();
    mBuffer = vk::Buffer();

    if (allocInfo.usage == 0)
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vk::BufferCreateInfo bufferCreateInfo { 
        .sType = vk::StructureType::eBufferCreateInfo,
        .size = size,
        .usage = usage
    };
    VkBuffer raw;
    VmaAllocationInfo info;

    VkResult result = vmaCreateBuffer(allocator->handle(), bufferCreateInfo, &allocInfo, &raw, &mAllocation, &info); 
    if (result != VK_SUCCESS) {
        throw std::runtime_error("vmaCreateBuffer failed!");
    }

    mBuffer = raw;
    mMapped = info.pMappedData;
}

void Buffer::swap(Buffer& o) noexcept {
    std::swap(mAllocator,  o.mAllocator);
    std::swap(mAllocation, o.mAllocation);
    std::swap(mBuffer,     o.mBuffer);
    std::swap(mMapped,     o.mMapped);
    std::swap(mSize,       o.mSize);
}

void Buffer::destroy() {
    if (mBuffer) {
        vmaDestroyBuffer(mAllocator->handle(), mBuffer, mAllocation);
        mBuffer = nullptr;
    }
}

void Buffer::copyMemory(const void *data, vk::DeviceSize size){
    vmaCopyMemoryToAllocation(mAllocator->handle(), data, mAllocation, 0, size);
}

void Buffer::copyMemoryStaged(
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

    commandBuffer.copyBuffer(
        stagingBuffer.mBuffer,
        mBuffer,
        vk::BufferCopy{.size = size}
    );

    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();

    std::cout << "Done copying staged buffer!!" << std::endl;
}

}