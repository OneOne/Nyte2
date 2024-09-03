#pragma once

#include "vulkan/vulkan_core.h"

class VulkanHelper
{
public:
	static VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice _physicalDevice);
};

