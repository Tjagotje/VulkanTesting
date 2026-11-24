#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

struct VmaAllocator_T;    // forward declarations: avoids including the huge VMA header here
struct VmaAllocation_T;
struct VmaAllocationCreateInfo;

namespace ht {




class Buffer {
public:
    Buffer() = default;
    Buffer(VmaAllocator_T* allocator, vk::DeviceSize size,
        vk::BufferUsageFlags usage);
    Buffer(VmaAllocator_T* allocator, vk::DeviceSize size,
        vk::BufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& o) noexcept;
    Buffer& operator=(Buffer&& o) noexcept;

    vk::Buffer     handle()  const { return mBuffer; } 
    void*          mapped()  const { return mMapped; }
    vk::DeviceSize size()    const { return mSize; }
    vk::DeviceAddress address(vk::Device device) const {
        return device.getBufferAddress({.buffer = mBuffer});
    };

    void copyMemory(const void* data, vk::DeviceSize size);

private:
    void destroy();
    void swap(Buffer& o) noexcept;

    VmaAllocator_T*  mAllocator  = nullptr;
    VmaAllocation_T* mAllocation = nullptr;
    vk::Buffer       mBuffer;
    void*            mMapped     = nullptr;
    vk::DeviceSize   mSize       = 0;
};

}