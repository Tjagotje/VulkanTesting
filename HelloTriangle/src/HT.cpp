#include "HT.hpp"

#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

void HelloTriangle::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangle::mainLoop() {

    constexpr float ANGLE = 0.03f;

    using clock = std::chrono::steady_clock;
    auto fpsLastTime = clock::now();
    int frameCount = 0;


    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            rot -= ANGLE;
        }
        else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            rot += ANGLE;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            dist -= 0.03f;
        } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            dist += 0.03f;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            height += 0.03f;
        } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            height -= 0.03f;
        }

        eye_pos = dist * glm::vec3(glm::cos(rot) * 2.0f, glm::sin(rot) * 2.0f, height);

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            break;
        }

        drawFrame();

        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // FPS counter
        ++frameCount;
        auto now = clock::now();
        std::chrono::duration<double> elapsed = now - fpsLastTime;
        if (elapsed.count() >= 1.0) {
            double fps = frameCount / elapsed.count();
            std::printf("FPS: %.1f (%.2f ms/frame)\n", fps, 1000.0 / fps);

            std::string title = "Hello Triangle - " + std::to_string(static_cast<int>(fps + 0.5)) + " FPS";
            glfwSetWindowTitle(window, title.c_str());

            frameCount = 0;
            fpsLastTime = now;
        }
    }

    device.waitIdle();
}


void HelloTriangle::drawFrame() {
    
    auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }
    device.resetFences(*inFlightFences[frameIndex]);

    auto [result, imageIndex] = swapChain.acquireNextImage(
        UINT64_MAX,
        *presentCompleteSemaphores[frameIndex],
        nullptr
    );
    recordCommandBuffer(imageIndex);

    updateUniformBuffer(frameIndex);

    vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask  = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers    = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores  = &*renderFinishedSemaphores[imageIndex]
    };

    queue.submit(submitInfo, *inFlightFences[frameIndex]);

    const vk::PresentInfoKHR presentInfoKHR {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*swapChain,
        .pImageIndices      = &imageIndex
    };

    result = queue.presentKHR(presentInfoKHR);

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangle::cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();

    vmaDestroyAllocator(allocator);
}


