#pragma once

#include "VulkanDevice.h"

class ImageHandler
{
public:
	ImageHandler(VulkanDevice& r_VulkanDevice);
	~ImageHandler();

	void CreateImage(VkImage& r_Image, VkDeviceMemory& r_ImageMemory,
		uint32_t a_Width, uint32_t a_Height, VkFormat a_Format, VkImageTiling a_Tiling, VkImageUsageFlags a_Usage, VkMemoryPropertyFlags a_Properties);
	void CreateTextureImage(VkImage& r_Image, VkDeviceMemory& r_ImageMemory,
		const char* a_FilePath, VkDeviceSize& r_ImageSize, int& r_TexWidth, int& r_TexHeight, int& r_TexChannels);
	void CopyBufferToImage(VkBuffer a_Buffer, VkImage a_Image, uint32_t a_Width, uint32_t a_Height);

	void TransitionImageLayout(VkImage a_Image, VkFormat a_Format, VkImageLayout a_OldLayout, VkImageLayout a_NewLayout);

	VkImageView CreateImageView(VkImage a_Image, VkFormat a_Format, VkImageAspectFlags a_AspectFlags);
	VkSampler CreateTextureSampler();
	bool HasStencilComponent(VkFormat a_Format);

private:

	VulkanDevice& rm_VulkanDevice;
};

