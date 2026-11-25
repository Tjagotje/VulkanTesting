#include "vk_mem_alloc.h"
#include "HT_memory.hpp"
#include <vulkan/vulkan_raii.hpp>

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

    mImgView = vk::raii::ImageView(device, viewInfo);
};

void Image::swap(Image& o) noexcept {
    std::swap(mAllocator, o.mAllocator);
    std::swap(mAllocation, o.mAllocation);
    std::swap(mImg, o.mImg);
    std::swap(mImgView, o.mImgView);
    std::swap(mMapped, o.mMapped);
}


void Image::destroy() {
    if (mImg) {
        vmaDestroyImage(mAllocator->handle(), mImg, mAllocation);
        mImgView = nullptr;
        mImg = nullptr;
    }
}

}