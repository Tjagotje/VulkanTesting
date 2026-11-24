#include "HT_memory.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace ht{

Buffer::Buffer(
    VmaAllocator_T* allocator,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    const VmaAllocationCreateInfo& allocInfo
) : mAllocator(allocator), mSize(size) {
    mAllocation = VmaAllocation();
    mBuffer = vk::Buffer();


    vk::BufferCreateInfo bufferCreateInfo { 
        .sType = vk::StructureType::eBufferCreateInfo,
        .size = size,
        .usage = usage
    };
    VkBuffer raw;
    VmaAllocationInfo info;

    VkResult result = vmaCreateBuffer(allocator, bufferCreateInfo, &allocInfo, &raw, &mAllocation, &info); 
    if (result != VK_SUCCESS) {
        throw std::runtime_error("vmaCreateBuffer failed!");
    }

    mBuffer = raw;
    mMapped = info.pMappedData;
}

Buffer::Buffer(Buffer&& o) noexcept {
    swap(o);
}

Buffer& Buffer::operator=(Buffer&& o) noexcept {
    if (this != &o) { destroy(); swap(o); }
    return *this;
}

Buffer::~Buffer() {
    destroy();
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
        vmaDestroyBuffer(mAllocator, mBuffer, mAllocation);
        mBuffer = nullptr;
    }
}

void Buffer::copyMemory(const void *data, vk::DeviceSize size){
    vmaCopyMemoryToAllocation(mAllocator, data, mAllocation, 0, size);
}


}