#pragma once

// stl
#include <optional>
#include <vector>
#include <array>
#include <chrono>
#include <iostream>
#include <string>

// vulkan
#include "vulkan/vulkan_core.h"

//// glfw
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>


#include "Common.h"
//#include "VertexBasic.h" // glm
#include "VertexModel.h" // glm
#include "FileHelper.h"

const std::string MODEL_PATH = "Resources/Models/viking_room.obj";
const std::string TEXTURE_PATH = "Resources/Textures/viking_room.png";


// VCR for Vulkan Check Result
inline std::string vkErrorString(VkResult errorCode)
{
    switch (errorCode)
    {
#define STR(r) case VK_ ##r: return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
        STR(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
#undef STR
    default:
        return std::string("UNKNOWN_ERROR");
    }
};

#define VCR(val, msg) if(val != VK_SUCCESS) {                                                                                           \
    std::cout << "Fatal : VkResult is \"" << vkErrorString(val) << "\" in " << __FILE__ << " at line " << __LINE__ << ".\n";    \
    throw std::runtime_error(msg);                                                                                                      \
}


namespace Nyte
{
    class Engine;


    struct QueueFamilyIndices
    {
        std::optional<u32> graphicsFamily;
        std::optional<u32> presentFamily;
        std::optional<u32> computeFamily;
        std::optional<u32> transferFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
        }
    };
    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct alignas(16) UBO_ModelViewProj // Uniform buffer object
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    class VulkanHelper
    {
    public:
        static u32 findMemoryType(VkDevice _device, VkPhysicalDevice _physicalDevice, VkMemoryRequirements _memoryRequirements, VkMemoryPropertyFlags _properties)
        {
            VkPhysicalDeviceMemoryProperties physicalMemoryProperties;
            vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &physicalMemoryProperties);
            for (u32 i = 0; i < physicalMemoryProperties.memoryTypeCount; i++)
            {
                if (_memoryRequirements.memoryTypeBits & (1 << i) && (physicalMemoryProperties.memoryTypes[i].propertyFlags & _properties) == _properties)
                {
                    return i;
                }
            }
            throw std::runtime_error("No memory type fit the given buffer.");
        }

        static VkCommandBuffer beginSingleTimeCommands(VkDevice _device, VkCommandPool _commandPool)
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = _commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }
        static void endSingleTimeCommands(
            VkDevice _device,
            VkCommandPool _commandPool,
            VkQueue _queue,
            VkCommandBuffer _commandBuffer,
            u32 _signalSemaphoreCount = 0, 
            VkSemaphore* _signalSemaphore = nullptr, 
            u32 _waitSemaphoreCount = 0, 
            VkSemaphore* _waitSemaphore = nullptr, 
            VkPipelineStageFlags* _waitStageMask = nullptr)
        {
            vkEndCommandBuffer(_commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &_commandBuffer;

            if (_signalSemaphoreCount > 0 && _signalSemaphore != nullptr)
            {
                submitInfo.signalSemaphoreCount = _signalSemaphoreCount;
                submitInfo.pSignalSemaphores = _signalSemaphore;
            }
            if (_waitSemaphoreCount > 0 && _waitSemaphore != nullptr)
            {
                submitInfo.waitSemaphoreCount = _waitSemaphoreCount;
                submitInfo.pWaitSemaphores = _waitSemaphore;
                submitInfo.pWaitDstStageMask = _waitStageMask;
            }

            vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(_queue);

            vkFreeCommandBuffers(_device, _commandPool, 1, &_commandBuffer);
        }
    };

    struct Buffer
    {
        VkBuffer m_buffer;
        VkDeviceMemory m_bufferDeviceMemory;

        VkDeviceSize m_size;
        VkBufferUsageFlags m_usage;
        VkMemoryPropertyFlags m_properties;

        inline void createBuffer(VkDevice _device, VkPhysicalDevice _physicalDevice)
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = m_size;
            bufferInfo.usage = m_usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VCR(vkCreateBuffer(_device, &bufferInfo, nullptr, &m_buffer), "Failed to create buffer.");

            VkMemoryRequirements memoryRequirements;
            vkGetBufferMemoryRequirements(_device, m_buffer, &memoryRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memoryRequirements.size;
            allocInfo.memoryTypeIndex = VulkanHelper::findMemoryType(_device, _physicalDevice, memoryRequirements, m_properties);
            VCR(vkAllocateMemory(_device, &allocInfo, nullptr, &m_bufferDeviceMemory), "Failed to allocate buffer device memory.");

            vkBindBufferMemory(_device, m_buffer, m_bufferDeviceMemory, 0);
        }
    };

    struct ImageAttachment
    {
        VkImage m_image;
        VkDeviceMemory m_imageDeviceMemory;
        VkImageView m_imageView;

        VkFormat m_format;
        VkExtent2D m_extent;
        u32 m_mipLevels;
        VkSampleCountFlagBits m_sampleCount;

        VkImageUsageFlags m_usage;
        VkImageAspectFlags m_aspectMask;
        VkClearValue m_clearValue;


        static ImageAttachment colorAttachment()
        {
            ImageAttachment attachment;
            attachment.m_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            attachment.m_aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            attachment.m_clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // black opaque
            return attachment;
        }
        static ImageAttachment depthAttachment()
        {
            ImageAttachment attachment;
            attachment.m_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            attachment.m_aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            attachment.m_clearValue.depthStencil.depth = 1.0f; // infinite depth 
            attachment.m_clearValue.depthStencil.stencil = 1; // 0 stencil
            return attachment;
        }

        inline void createImageAttachment(VkDevice _device, VkPhysicalDevice _physicalDevice)
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = m_extent.width;
            imageInfo.extent.height = m_extent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = m_mipLevels;
            imageInfo.samples = m_sampleCount;
            imageInfo.arrayLayers = 1;
            imageInfo.format = m_format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // not usable by the GPU and the very first transition will discard the texels
            imageInfo.usage = m_usage;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // graphics queue exclusive

            VCR(vkCreateImage(_device, &imageInfo, nullptr, &m_image), "Failed to create image.");

            // Allocate device memory
            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements(_device, m_image, &memoryRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memoryRequirements.size;
            allocInfo.memoryTypeIndex = VulkanHelper::findMemoryType(_device, _physicalDevice, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VCR(vkAllocateMemory(_device, &allocInfo, nullptr, &m_imageDeviceMemory), "Failed to allocate image device memory.");

            // Bind image and device memory
            vkBindImageMemory(_device, m_image, m_imageDeviceMemory, 0);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_format;
            viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
            viewInfo.subresourceRange.aspectMask = m_aspectMask;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = m_mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VCR(vkCreateImageView(_device, &viewInfo, nullptr, &m_imageView), "Failed to create image view.");
        }
        inline void destroyImageAttachment(VkDevice _device)
        {
            vkDestroyImageView(_device, m_imageView, nullptr);
            vkFreeMemory(_device, m_imageDeviceMemory, nullptr);
            vkDestroyImage(_device, m_image, nullptr);
        }

        inline VkAttachmentDescription getAttachmentDescription(VkImageLayout _layout)
        {
            VkAttachmentDescription attachmentDescription;
            attachmentDescription.flags = 0;
            attachmentDescription.format = m_format;
            attachmentDescription.samples = m_sampleCount;
            attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear color framebuffer before pass
            attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // no stencil == no care
            attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescription.finalLayout = _layout;

            return attachmentDescription;
        }
        inline VkAttachmentReference  getAttachmentDescriptionRef(u32 _attachmentIndex, VkImageLayout _layout)
        {
            VkAttachmentReference attachmentReference;
            attachmentReference.attachment = _attachmentIndex; // layout(location = _attachmentIndex)
            attachmentReference.layout = _layout;

            return attachmentReference;
        }

        inline void loadImageFromFile(VkDevice _device, VkPhysicalDevice _physicalDevice, std::string _filePath, VkSampleCountFlagBits _sampleCount, VkCommandPool _commandPool, VkQueue _queue)
        {
            // Load image from file
            RawImage rawImage;
            rawImage.path = _filePath;
            FileHelper::loadImage(rawImage);

            // Create a staging buffer for transfer
            Buffer stagingBuffer;
            stagingBuffer.m_size = (VkDeviceSize)rawImage.size;
            stagingBuffer.m_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stagingBuffer.m_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            stagingBuffer.createBuffer(_device, _physicalDevice);

            void* data;
            vkMapMemory(_device, stagingBuffer.m_bufferDeviceMemory, 0, (VkDeviceSize)rawImage.size, 0, &data);
            memcpy(data, rawImage.data, (u32)rawImage.size);
            vkUnmapMemory(_device, stagingBuffer.m_bufferDeviceMemory);

            // Unload image data
            FileHelper::unloadImage(rawImage);

            m_format = VK_FORMAT_R8G8B8A8_SRGB;
            m_extent = { (u32)rawImage.width, (u32)rawImage.height };
            m_mipLevels = rawImage.mipLevels;
            m_sampleCount = VK_SAMPLE_COUNT_1_BIT;// _sampleCount;

            // Add transfer to usage
            m_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            createImageAttachment(_device, _physicalDevice);


            VkCommandBuffer commandBuffer = VulkanHelper::beginSingleTimeCommands(_device, _commandPool);

            // Transition to "transfer layout"
            {
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = m_image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = m_mipLevels;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;

                barrier.srcAccessMask = VK_ACCESS_NONE;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );
            }

            // Copy staging to image
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { (u32)rawImage.width, (u32)rawImage.height, 1 };
            vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.m_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            // Generate mipmaps
            {
                // Check image format support filtering
                VkFormatProperties formatProperties;
                vkGetPhysicalDeviceFormatProperties(_physicalDevice, m_format, &formatProperties);

                if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
                {
                    throw std::runtime_error("Image format does not support linear filtering.");
                }

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.image = m_image;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.subresourceRange.levelCount = 1;

                i32 mipWidth = rawImage.width;
                i32 mipHeight = rawImage.height;

                for (uint32_t i = 1; i < m_mipLevels; i++)
                {
                    barrier.subresourceRange.baseMipLevel = i - 1;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                    // Barrier from transfer dst layout to transfer src (on graphics queue)
                    vkCmdPipelineBarrier(
                        commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

                    VkImageBlit blit{};
                    blit.srcOffsets[0] = { 0, 0, 0 };
                    blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = 1;
                    blit.dstOffsets[0] = { 0, 0, 0 };
                    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.layerCount = 1;

                    // Blit mip level
                    vkCmdBlitImage(
                        commandBuffer,
                        m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &blit,
                        VK_FILTER_LINEAR);

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    // Barrier from transfer src layout to shader read only (on graphics queue)
                    vkCmdPipelineBarrier(
                        commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }


                barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                // Barrier from transfer dst layout to shader read only (on graphics queue)
                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);
            }

            //// Transition to "shader ready layout"
            //{
            //    VkImageMemoryBarrier barrier{};
            //    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            //    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            //    barrier.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
            //    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            //    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            //    barrier.image = m_image;
            //    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            //    barrier.subresourceRange.baseMipLevel = 0;
            //    barrier.subresourceRange.levelCount = m_mipLevels;
            //    barrier.subresourceRange.baseArrayLayer = 0;
            //    barrier.subresourceRange.layerCount = 1;
            //
            //    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            //    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            //
            //    vkCmdPipelineBarrier(
            //        commandBuffer,
            //        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            //        0,
            //        0, nullptr,
            //        0, nullptr,
            //        1, &barrier
            //    );
            //}

            VulkanHelper::endSingleTimeCommands(_device, _commandPool, _queue, commandBuffer);
        }
    };

    struct RenderPass
    {
        VkRenderPass m_renderPass;

        // must be ordered according to attachment index
        std::vector<VkAttachmentReference> m_colorAttachmentReferences;
        std::vector<VkAttachmentReference> m_depthStencilAttachmentReferences;
        std::vector<VkAttachmentReference> m_resolveAttachmentReferences;
        std::vector<VkAttachmentDescription> m_attachmentDescriptions;
        std::vector<VkSubpassDependency> m_dependencies;


        inline void createRenderPass(VkDevice _device)
        {
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = (u32)m_colorAttachmentReferences.size();
            subpass.pColorAttachments = m_colorAttachmentReferences.data();
            subpass.pDepthStencilAttachment = m_depthStencilAttachmentReferences.data();
            subpass.pResolveAttachments = m_resolveAttachmentReferences.data();

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = (u32)m_attachmentDescriptions.size();
            renderPassInfo.pAttachments = m_attachmentDescriptions.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = (u32)m_dependencies.size();
            renderPassInfo.pDependencies = m_dependencies.data();

            VCR(vkCreateRenderPass(_device, &renderPassInfo, nullptr, &m_renderPass), "Failed to create render pass.");
        }
        inline void destroyRenderPass(VkDevice _device)
        {
            vkDestroyRenderPass(_device, m_renderPass, nullptr);
        }

        inline static VkSubpassDependency colorDependency()
        {
            VkSubpassDependency dependency;
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            dependency.dependencyFlags = 0;
            return dependency;
        }
        inline static VkSubpassDependency depthDependency()
        {
            VkSubpassDependency dependency;
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
            dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            dependency.dependencyFlags = 0;
            return dependency;
        }
    };

    struct Framebuffer
    {
        VkFramebuffer m_framebuffer;

        std::vector<ImageAttachment> m_attachments;
        RenderPass m_renderPass;
        VkExtent2D m_extent;

        inline void createFramebuffer(VkDevice _device)
        {
            std::vector<VkImageView> attachments;
            for (const ImageAttachment& attachment : m_attachments)
                attachments.push_back(attachment.m_imageView);

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass.m_renderPass;
            framebufferInfo.attachmentCount = (u32)attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_extent.width;
            framebufferInfo.height = m_extent.height;
            framebufferInfo.layers = 1;

            VCR(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &m_framebuffer), "Failed to create framebuffer.");
        }
        inline void destroyFramebuffer(VkDevice _device)
        {
            vkDestroyFramebuffer(_device, m_framebuffer, nullptr);
        }
    };

    struct ShaderStage
    {
        VkShaderModule m_shaderModule;

        std::string m_path;
        VkShaderStageFlagBits m_stageFlag;

        static ShaderStage vertexShader()
        {
            ShaderStage stage;
            stage.m_stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
            return stage;
        }
        static ShaderStage fragmentShader()
        {
            ShaderStage stage;
            stage.m_stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
            return stage;
        }

        inline void createShader(VkDevice _device)
        {
            std::vector<octet> code = FileHelper::readFile(m_path.c_str());

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());


            VCR(vkCreateShaderModule(_device, &createInfo, nullptr, &m_shaderModule), "Failed to create shader module.");
        }
        inline void destroyShader(VkDevice _device)
        {
            vkDestroyShaderModule(_device, m_shaderModule, nullptr);
        }

        inline VkPipelineShaderStageCreateInfo getStageCreateInfo() const
        {
            VkPipelineShaderStageCreateInfo stageInfo = {};
            stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageInfo.pNext = nullptr;
            stageInfo.stage = m_stageFlag;
            stageInfo.module = m_shaderModule;
            stageInfo.pName = "main";
            stageInfo.pSpecializationInfo = nullptr;
            return stageInfo;
        }
    };

    struct InputAttribut
    {
        VkFormat m_format;
        u32 m_size;
    };
    struct VertexDescription
    {
        std::vector<VkVertexInputAttributeDescription> m_inputAttributeDescriptions;
        VkVertexInputBindingDescription m_bindingDescription;

        u32 m_bindingIndex = 0;
        std::vector<InputAttribut> m_inputs;


        VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0; // binding index 
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        inline void setup()
        {
            u32 index = 0;
            u32 offset = 0;
            for (InputAttribut input : m_inputs)
            {
                VkVertexInputAttributeDescription inputAttributeDescription;
                inputAttributeDescription.binding = m_bindingIndex;
                inputAttributeDescription.location = index;
                inputAttributeDescription.format = input.m_format;
                inputAttributeDescription.offset = offset;

                m_inputAttributeDescriptions.push_back(inputAttributeDescription);
                index++;
                offset += input.m_size;
            }

            m_bindingDescription.binding = m_bindingIndex;
            m_bindingDescription.stride = offset;
            m_bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        }
    };

    struct DescriptorSetLayout
    {
        VkDescriptorSetLayout m_descriptorSetLayout;

        std::vector<VkDescriptorSetLayoutBinding> m_bindings;

        inline void addUniformBufferBinding(VkShaderStageFlags _stageFlags)
        {
            VkDescriptorSetLayoutBinding binding;
            binding.binding = (u32)m_bindings.size();
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = _stageFlags;
            binding.pImmutableSamplers = nullptr; // Optional

            m_bindings.push_back(binding);
        }
        inline void addSamplerBinding()
        {
            VkDescriptorSetLayoutBinding binding;
            binding.binding = (u32)m_bindings.size();
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            binding.pImmutableSamplers = nullptr; // Optional

            m_bindings.push_back(binding);
        }

        inline void createDescriptorSetLayout(VkDevice _device)
        {
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = (u32)m_bindings.size();
            layoutInfo.pBindings = m_bindings.data();

            VCR(vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &m_descriptorSetLayout), "Faild to create descriptor set layout.");
        }
        inline void destroyDescriptorSetLayout(VkDevice _device)
        {
            vkDestroyDescriptorSetLayout(_device, m_descriptorSetLayout, nullptr);
        }
    };
    struct DescriptorSets
    {
        struct WriteInfo
        {
            std::optional<VkDescriptorBufferInfo> m_bufferInfo;
            std::optional<VkDescriptorImageInfo> m_imageInfo;
        };
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkDescriptorPool m_descriptorPool;

        DescriptorSetLayout m_descriptorSetLayout;

        std::vector<VkWriteDescriptorSet> m_writeDescriptorSets;
        std::vector<WriteInfo> m_infos;

        inline void addWriteBufferDescriptorSet(VkDescriptorSet _dstSet, u32 _bindingIndex, VkDescriptorType _descriptorType, VkBuffer _buffer, u32 _bufferOffset, u32 _bufferRange)
        {
            WriteInfo writeInfo;
            writeInfo.m_bufferInfo = { _buffer, _bufferOffset, _bufferRange };
            m_infos.push_back(writeInfo);

            VkWriteDescriptorSet descriptorWrite;
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = nullptr;
            descriptorWrite.dstSet = _dstSet;
            descriptorWrite.dstBinding = _bindingIndex;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = _descriptorType;
            descriptorWrite.descriptorCount = 1;

            m_writeDescriptorSets.push_back(descriptorWrite);
        }
        inline void addWriteImageDescriptorSet(VkDescriptorSet _dstSet, u32 _bindingIndex, VkDescriptorType _descriptorType, VkSampler _sampler, VkImageView _imageView, VkImageLayout _imageLayout)
        {
            WriteInfo writeInfo;
            writeInfo.m_imageInfo = { _sampler, _imageView, _imageLayout };
            m_infos.push_back(writeInfo);

            VkWriteDescriptorSet descriptorWrite;
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = nullptr;
            descriptorWrite.dstSet = _dstSet;
            descriptorWrite.dstBinding = _bindingIndex;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = _descriptorType;
            descriptorWrite.descriptorCount = 1;

            m_writeDescriptorSets.push_back(descriptorWrite);
        }

        inline void allocateDescriptorSets(VkDevice _device, u32 _allocateCount)
        {
            std::vector<VkDescriptorPoolSize> poolSizes{};
            for (VkDescriptorSetLayoutBinding binding : m_descriptorSetLayout.m_bindings)
            {
                poolSizes.emplace_back(binding.descriptorType, _allocateCount);
            }

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = (u32)poolSizes.size();
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = _allocateCount;

            //VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT

            VCR(vkCreateDescriptorPool(_device, &poolInfo, nullptr, &m_descriptorPool), "Failed to create descriptor pool.");


            std::vector<VkDescriptorSetLayout> layouts(_allocateCount, m_descriptorSetLayout.m_descriptorSetLayout);

            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_descriptorPool;
            allocInfo.descriptorSetCount = _allocateCount;
            allocInfo.pSetLayouts = layouts.data();

            m_descriptorSets.resize(_allocateCount);

            VkResult r = vkAllocateDescriptorSets(_device, &allocInfo, m_descriptorSets.data());
            VCR(r, "Failed to create descriptor sets.");
        }
        inline void freeDescriptorSets(VkDevice _device)
        {
            //vkFreeDescriptorSets(...m_descriptorSets...) is already done when destroying descriptor pool
            vkDestroyDescriptorPool(_device, m_descriptorPool, nullptr);
        }
        inline void updateDescriptorSets(VkDevice _device)
        {
            for (u32 i = 0; i < m_writeDescriptorSets.size(); ++i)
            {
                VkWriteDescriptorSet& writeDescriptorSet = m_writeDescriptorSets[i];
                WriteInfo& info = m_infos[i];

                if (info.m_bufferInfo.has_value())
                    writeDescriptorSet.pBufferInfo = &info.m_bufferInfo.value();
                else if (info.m_imageInfo.has_value())
                    writeDescriptorSet.pImageInfo = &info.m_imageInfo.value();
            }

            vkUpdateDescriptorSets(_device, (u32)m_writeDescriptorSets.size(), m_writeDescriptorSets.data(), 0, nullptr);
            m_writeDescriptorSets.clear();
            m_infos.clear();
        }
    };

    struct PipelineLayout
    {
        VkPipelineLayout m_pipelineLayout;

        std::vector<DescriptorSetLayout> m_descriptorSetLayouts;

        inline void createPipelineLayout(VkDevice _device)
        {
            std::vector<VkDescriptorSetLayout> setLayouts;
            for (const DescriptorSetLayout& setLayout : m_descriptorSetLayouts)
                setLayouts.push_back(setLayout.m_descriptorSetLayout);

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = (u32)setLayouts.size();
            pipelineLayoutInfo.pSetLayouts = setLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

            VCR(vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout), "Failed to create pipeline layout.");
        }
        inline void destroyPipelineLayout(VkDevice _device)
        {
            vkDestroyPipelineLayout(_device, m_pipelineLayout, nullptr);
        }
    };
    struct Pipeline
    {
        VkPipeline m_pipeline;

        std::vector<ShaderStage> m_shaders;
        VertexDescription m_vertexDescription;
        VkPrimitiveTopology m_topology;
        VkExtent2D m_extent;
        VkSampleCountFlagBits m_sampleCount;
        RenderPass m_renderPass;
        PipelineLayout m_pipelineLayout;
        bool m_depthTestEnable = true;

        inline void createPipeline(VkDevice _device)
        {
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
            for (const ShaderStage& shader : m_shaders)
                shaderStages.push_back(shader.getStageCreateInfo());


            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            if (m_vertexDescription.m_inputAttributeDescriptions.size() > 0)
            {
                vertexInputInfo.vertexBindingDescriptionCount = 1;
                vertexInputInfo.pVertexBindingDescriptions = &m_vertexDescription.m_bindingDescription;
                vertexInputInfo.vertexAttributeDescriptionCount = (u32)m_vertexDescription.m_inputAttributeDescriptions.size();
                vertexInputInfo.pVertexAttributeDescriptions = m_vertexDescription.m_inputAttributeDescriptions.data();
            }

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = m_topology;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)m_extent.width;
            viewport.height = (float)m_extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = m_extent;

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // fill, edges or points
            rasterizer.lineWidth = 1.0f; // default value (otherwise require "wideLines" extension)
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //VK_CULL_MODE_FRONT_AND_BACK; //VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // careful, we use ccw here due to glm->vulkan conversion in projection matrix
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f; // Optional
            rasterizer.depthBiasClamp = 0.0f; // Optional
            rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = m_sampleCount;
            multisampling.minSampleShading = 1.0f; // Optional
            multisampling.pSampleMask = nullptr; // Optional
            multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
            multisampling.alphaToOneEnable = VK_FALSE; // Optional

            VkPipelineDepthStencilStateCreateInfo depthAndStencil{};
            depthAndStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            if(m_depthTestEnable)
            {
                depthAndStencil.depthTestEnable = VK_TRUE;
                depthAndStencil.depthWriteEnable = VK_TRUE;
                depthAndStencil.depthCompareOp = VK_COMPARE_OP_LESS;
                depthAndStencil.depthBoundsTestEnable = VK_FALSE;
                depthAndStencil.minDepthBounds = 0.0f; // Optional
                depthAndStencil.maxDepthBounds = 1.0f; // Optional
                depthAndStencil.stencilTestEnable = VK_FALSE;
                depthAndStencil.front = {}; // Optional
                depthAndStencil.back = {}; // Optional
            }
            else
            {
                depthAndStencil.depthTestEnable = VK_FALSE;
            }

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

            u32 colorAttachmentCount = (u32)m_renderPass.m_colorAttachmentReferences.size();
            std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(colorAttachmentCount, colorBlendAttachment);

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
            colorBlending.attachmentCount = colorAttachmentCount;
            colorBlending.pAttachments = colorBlendAttachments.data();
            colorBlending.blendConstants[0] = 0.0f; // Optional
            colorBlending.blendConstants[1] = 0.0f; // Optional
            colorBlending.blendConstants[2] = 0.0f; // Optional
            colorBlending.blendConstants[3] = 0.0f; // Optional

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.pNext = nullptr;
            pipelineInfo.stageCount = (u32)shaderStages.size();
            pipelineInfo.pStages = shaderStages.data();
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pTessellationState = nullptr;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthAndStencil;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = nullptr; // Optional
            pipelineInfo.layout = m_pipelineLayout.m_pipelineLayout;
            pipelineInfo.renderPass = m_renderPass.m_renderPass;
            pipelineInfo.subpass = 0; // subpass index
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional (pipeline inheritance)
            pipelineInfo.basePipelineIndex = -1; // Optional
            pipelineInfo.flags = 0;

            VCR(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline), "Failed to create graphics pipeline(s).");
        }
        inline void destroyPipeline(VkDevice _device)
        {
            vkDestroyPipeline(_device, m_pipeline, nullptr);
        }
    };

    struct CommandBuffers
    {
        std::vector<VkCommandBuffer> m_commandBuffers;

        Pipeline m_pipeline;
        Framebuffer m_framebuffer;

        inline void allocateCommands(VkDevice _device, VkCommandPool _commandPool, u32 _count)
        {
            m_commandBuffers.resize(_count);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = _commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = _count;

            VCR(vkAllocateCommandBuffers(_device, &allocInfo, m_commandBuffers.data()), "Failed to allocate command buffers.");
        }
        inline void freeCommands(VkDevice _device, VkCommandPool _commandPool)
        {
            vkFreeCommandBuffers(_device, _commandPool, (u32)m_commandBuffers.size(), m_commandBuffers.data());
        }

        inline void beginPass(u32 _index)
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            VCR(vkBeginCommandBuffer(m_commandBuffers[_index], &beginInfo), "Failed to begin command buffer.");

            std::vector<VkClearValue> clearValues;
            for (const ImageAttachment& imageAttachment : m_framebuffer.m_attachments)
                clearValues.push_back(imageAttachment.m_clearValue);

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_pipeline.m_renderPass.m_renderPass;
            renderPassInfo.framebuffer = m_framebuffer.m_framebuffer;
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = m_pipeline.m_extent;
            renderPassInfo.clearValueCount = (u32)clearValues.size();
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(m_commandBuffers[_index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // VK_SUBPASS_CONTENTS_INLINE = render pass commands are directly included into the command buffer
            vkCmdBindPipeline(m_commandBuffers[_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.m_pipeline);
        }
        inline void endPass(u32 _index)
        {
            vkCmdEndRenderPass(m_commandBuffers[_index]);

            VCR(vkEndCommandBuffer(m_commandBuffers[_index]), "Failed to end command buffer.");
        }

        VkCommandBuffer& operator[](const u32 _index)
        {
            return m_commandBuffers[_index];
        }
    };
    struct Semaphores
    {
        std::vector<VkSemaphore> m_semaphores;
        
        inline void createSemaphores(VkDevice _device, u32 _count)
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo{};
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            m_semaphores.resize(_count);
            for (u32 i = 0; i < _count; ++i)
            {
                VCR(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &m_semaphores[i]), "Failed to create semaphore");
            }
        }
        inline void destroySemaphores(VkDevice _device)
        {
            for(VkSemaphore& semaphore : m_semaphores)
                vkDestroySemaphore(_device, semaphore, nullptr);
        }

        VkSemaphore& operator[](const u32 _index)
        {
            return m_semaphores[_index];
        }
    };

    struct GBuffer
    {
        ImageAttachment m_colorAttachment;
        ImageAttachment m_normalAttachment;
        ImageAttachment m_specGlossAttachment;
        ImageAttachment m_depthAttachment;

        RenderPass m_renderPass;
        Framebuffer m_framebuffer;

        ShaderStage m_vertexShader;
        ShaderStage m_fragmentShader;

        DescriptorSetLayout m_descriptorSetLayout;
        DescriptorSets m_descriptorSets;

        PipelineLayout m_pipelineLayout;
        Pipeline m_pipeline;

        CommandBuffers m_cmdBuffers;
    };

    struct DeferredResolve
    {
        RenderPass m_renderPass;
        std::vector<Framebuffer> m_framebuffers;

        ShaderStage m_vertexShader;
        ShaderStage m_fragmentShader;

        DescriptorSetLayout m_descriptorSetLayout;
        DescriptorSets m_descriptorSets;

        PipelineLayout m_pipelineLayout;
        Pipeline m_pipeline;

        CommandBuffers m_cmdBuffers;
    };


    class Engine 
    {
    public:
        void createInstance(std::vector<const char*> _requiredExtensions);
        void destroyInstance();

        void init();
        void deinit();

        void drawFrame();
        void idle();

        void resizeWindow(int _width, int _height);

        VkInstance& getInstance() { return m_instance; };
        void setSurface(VkSurfaceKHR* _surface) { m_surface = _surface; };

    private:
#pragma region PhysicalDevice
        void pickPhysicalDevice();
        SwapchainSupportDetails retrieveSwapchainSupportDetails(VkPhysicalDevice _device, VkSurfaceKHR _window);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _device, VkSurfaceKHR _window);
        VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice _physicalDevice);
        bool isPhysicalDeviceSuitable(VkPhysicalDevice _device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice _device);

#if _DEBUG
        bool checkValidationLayerSupport();
        void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& _createInfo);
        void setupDebugMessenger();
#endif

#pragma endregion PhysicalDevice

        void createLogicalDevice();

#pragma region Swapchain
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& _availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& _availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities);
        void createSwapchain();
        void createImageViews();
        void destroySwapchain();
        void recreateSwapchain();
#pragma endregion Swapchain

#pragma region Common
        VkCommandBuffer beginSingleTimeCommands(VkCommandPool _commandPool);
        void endSingleTimeCommands(VkCommandPool _commandPool, VkQueue _queue, VkCommandBuffer _commandBuffer, u32 _signalSemaphoreCount = 0, VkSemaphore* _signalSemaphore = nullptr, u32 _waitSemaphoreCount = 0, VkSemaphore* _waitSemaphore = nullptr, VkPipelineStageFlags* _waitStageMask = nullptr);
        void createBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _properties, VkBuffer& _buffer, VkDeviceMemory& _bufferDeviceMemory);
        void createConcurrentBuffer(VkDeviceSize _size, VkBufferUsageFlags _usage, u32 _sharedQueueCount, u32* _sharedQueueIndices, VkMemoryPropertyFlags _properties, VkBuffer& _buffer, VkDeviceMemory& _bufferDeviceMemory);
        void copyBuffer(VkBuffer _srcBuffer, VkBuffer _dstBuffer, VkDeviceSize _size);
        void createImage(u32 _width, u32 _height, u32 _mipLevels, VkSampleCountFlagBits _sampleCount, VkFormat _format, VkImageTiling _tiling, VkImageUsageFlags _usage, u32 _sharedQueueCount, u32* _sharedQueueIndices, VkMemoryPropertyFlags _properties, VkImage& _image, VkDeviceMemory& _imageDeviceMemory);
        void transitionImageLayoutToTransfer(VkImage _image, u32 _mipLevels);
        void transitionImageLayoutToGraphics(VkImage _image, u32 _mipLevels, VkImageAspectFlags _aspectMask, VkImageLayout _newLayout, VkAccessFlags _dstAccessMask, VkPipelineStageFlags _dstStageMask);
        void transitionImageLayoutFromTransferToGraphics(VkImage _image, u32 _mipLevels);
        void copyBufferToImage(VkBuffer _buffer, VkImage _image, u32 _width, u32 _height);
        void createImageView(VkImage _image, VkFormat _format, u32 _mipLevels, VkImageAspectFlags _aspectMask, VkImageView& _imageView);
        void generateMipmaps(VkImage _image, VkFormat _format, u32 _texWidth, u32 _texHeight, u32 _mipLevels);
#pragma endregion Common

        void createCommandPools();

        void createOffscreenGBuffer();
        void destroyOffscreenGBuffer();

        void createDeferredPipepline();
        void destroyDeferredPipeline();



        VkFormat findSupportedFormat(const std::vector<VkFormat>& _candidates, VkImageTiling _tiling, VkFormatFeatureFlags _features);
        bool hasStencilComponent(VkFormat _format);
        VkFormat findDepthFormat();

        void loadModel();

        u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        //void createTextureImage();
        //void createTextureImageView();
        void createTextureSampler();

        void createSemaphoresAndFences();
        void destroySemaphoresAndFences();


        void updateUniformBuffer(u32 _currentImage);

    private:
        static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
        u32 m_windowWidth;
        u32 m_windowHeight;

        VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VkInstance m_instance;
        VkSurfaceKHR* m_surface;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

        VkDevice m_logicalDevice;
        QueueFamilyIndices m_queueFamilyIndices;
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;
        VkQueue m_transferQueue;

        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        VkFormat m_swapchainImageFormat;
        VkExtent2D m_swapchainExtent;

        VkCommandPool m_graphicsCommandPool;
        VkCommandPool m_transferCommandPool;


        GBuffer m_gbuffer;
        DeferredResolve m_deferred;

        Semaphores m_gbufferSemaphores;


        std::vector<Vertex> m_vertices;
        std::vector<u32> m_indices;
        VkBuffer m_vertexBuffer;
        VkDeviceMemory m_vertexBufferDeviceMemory;
        VkBuffer m_indexBuffer;
        VkDeviceMemory m_indexBufferDeviceMemory;

        std::vector<VkBuffer> m_uniformBuffers;
        std::vector<VkDeviceMemory> m_uniformBuffersDeviceMemory;

        std::vector<ImageAttachment> m_textures;

        RawImage m_texture;
        //VkImage m_textureImage;
        //VkDeviceMemory m_textureImageDeviceMemory;
        //VkImageView m_textureImageView;
        VkSampler m_textureSampler;

        // Note: Fences synchronize c++ calls with gpu operations
        //       Semaphores synchronize gpu operations with one another
        Semaphores m_imageAvailableSemaphores;
        Semaphores m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlightFences;
        u32 m_currentFrame = 0;

        bool m_framebufferResized = false;

#if _DEBUG
        VkDebugUtilsMessengerEXT m_callback;
#endif
    };
};

