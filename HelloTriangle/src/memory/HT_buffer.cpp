#include "HT_memory.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

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


}