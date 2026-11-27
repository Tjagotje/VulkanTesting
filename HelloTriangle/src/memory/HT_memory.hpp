#pragma once

#include "vulkan/vulkan.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

struct VmaAllocator_T;    // forward declarations: avoids including the huge VMA header here
struct VmaAllocation_T;
struct VmaAllocatorCreateInfo;
struct VmaAllocationCreateInfo;

namespace ht {

class Allocator {
public:
    Allocator() = default;
    explicit Allocator(VmaAllocatorCreateInfo info);
    ~Allocator();

    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

    Allocator(Allocator&& o) noexcept 
        : mAllocator(std::exchange(o.mAllocator, nullptr)) {}

    Allocator& operator=(Allocator&& other) noexcept;
     
    VmaAllocator_T* handle() const { return mAllocator; }

private:
    VmaAllocator_T* mAllocator = nullptr;
};


class Buffer {
public:
    Buffer() = default;
    Buffer(Allocator* allocator, vk::DeviceSize size,
        vk::BufferUsageFlags usage, VmaAllocationCreateInfo allocInfo);
    ~Buffer() {
        destroy();
    };

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& o) noexcept {
        swap(o);
    };
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) { destroy(); swap(o); };
        return *this;
    };

    vk::Buffer     handle()  const { return mBuffer; } 
    void*          mapped()  const { return mMapped; }
    vk::DeviceSize size()    const { return mSize; }
    vk::DeviceAddress address(vk::Device device) const {
        return device.getBufferAddress({.buffer = mBuffer});
    };

    void copyMemory(const void* data, vk::DeviceSize size);
    void copyMemoryStaged(
        const void* data,
        vk::DeviceSize size,
        const vk::raii::CommandPool& pool,
        const vk::raii::Device& device,
        const vk::raii::Queue& queue
    );

private:
    void destroy();
    void swap(Buffer& o) noexcept;

    Allocator*  mAllocator  = nullptr;
    VmaAllocation_T* mAllocation = nullptr;
    vk::Buffer       mBuffer;
    void*            mMapped     = nullptr;
    vk::DeviceSize   mSize       = 0;
};


class Image {
public:
    Image() = default;
    Image(
        Allocator* allocator,
        const vk::raii::Device& device,
        vk::ImageCreateInfo imageInfo,
        vk::ImageViewCreateInfo viewInfo,
        VmaAllocationCreateInfo allocInfo
    );
    ~Image() {
        destroy();
    };

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& o) noexcept {
        swap(o);
    };
    Image& operator=(Image&& o) noexcept {
        if (this != &o) { destroy(); swap(o); };
        return *this;
    };

    vk::Image *    pointer() { return &mImg; }
    vk::Image      handle()  const { return mImg; } 
    vk::ImageView  view()   { return mImgView; }
    void*          mapped()  const { return mMapped; }

    void copyMemoryStaged(
        const void *data,
        vk::DeviceSize size,
        const vk::raii::CommandPool& pool,
        const vk::raii::Device& device,
        const vk::raii::Queue& queue
    );

    void transition_layout(
        vk::raii::CommandBuffer& commandBuffer,
        vk::ImageLayout          old_layout,
        vk::ImageLayout          new_layout,
        vk::AccessFlags2         src_access_mask,
        vk::AccessFlags2         dst_access_mask,
        vk::PipelineStageFlags2  src_stage_mask,
        vk::PipelineStageFlags2  dst_stage_mask,
        vk::ImageAspectFlags     aspect_flags
    );

private:
    void destroy();
    void swap(Image& o) noexcept;

    Allocator*          mAllocator  = nullptr;
    VmaAllocation_T*    mAllocation = nullptr;
    vk::Image           mImg;
    vk::raii::ImageView mImgView    = nullptr; 
    void*               mMapped     = nullptr;
    vk::Extent3D        mExtent;
};


}
