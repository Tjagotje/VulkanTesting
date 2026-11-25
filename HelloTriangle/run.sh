#! /bin/bash
source ~/SDKs/Vulkan/default/setup-env.sh
cmake --build build -j$(nproc)
./build/HelloTriangle