#include "HT_memory.hpp"
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace ht{

Allocator::Allocator(VmaAllocatorCreateInfo info) {
    mAllocator = VK_NULL_HANDLE;

    VkResult result = vmaCreateAllocator(&info, &mAllocator);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("vmaCreateAllocator failed: " +
            std::to_string(result));
    }
}

Allocator::~Allocator() {
    vmaDestroyAllocator(mAllocator);
}

Allocator& Allocator::operator=(Allocator&& other) noexcept {
    if (this != &other) {
        if (mAllocator) {
            vmaDestroyAllocator(mAllocator);
        };

        mAllocator = std::exchange(other.mAllocator, nullptr);
    }
    return *this;
}

Buffer::Buffer(
    ht::Allocator* allocator,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    const VmaAllocationCreateInfo& allocInfo
) : mAllocator(allocator), mSize(size) {
    mAllocation = VmaAllocation();
    mBuffer = vk::Buffer();

    assert(allocator != nullptr);
    assert(allocator->handle() != nullptr);

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
        vmaDestroyBuffer(mAllocator->handle(), mBuffer, mAllocation);
        mBuffer = nullptr;
    }
}

void Buffer::copyMemory(const void *data, vk::DeviceSize size){
    vmaCopyMemoryToAllocation(mAllocator->handle(), data, mAllocation, 0, size);
}


}