#include "HT_memory.hpp"
#include "vk_mem_alloc.h"


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
}