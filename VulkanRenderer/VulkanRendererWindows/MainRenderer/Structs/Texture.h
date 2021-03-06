#pragma once
#pragma warning (push, 0)
#include <Vulkan/vulkan.h>
#pragma warning (pop)

constexpr uint32_t RGBASize = 4;

struct Texture
{
	//Metrics
	uint32_t texWidth = 0;
	uint32_t texHeight = 0;
	uint32_t texChannels = 0;

	const VkDeviceSize TexSize() const { return static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * static_cast<VkDeviceSize>(RGBASize); }

	#pragma region Vulkan
	void UpdateDescriptor()
	{
		descriptorInfo.sampler = textureSampler;
		descriptorInfo.imageView = textureImageView;
		descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void DeleteBuffers(VkDevice& r_Device)
	{
		vkDestroySampler(r_Device, textureSampler, nullptr);
		vkDestroyImageView(r_Device, textureImageView, nullptr);

		vkDestroyImage(r_Device, textureImage, nullptr);
		vkFreeMemory(r_Device, textureImageMemory, nullptr);
	}

	VkImage textureImage = VK_NULL_HANDLE;
	VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
	VkImageView textureImageView = VK_NULL_HANDLE;
	VkSampler textureSampler = VK_NULL_HANDLE;

	VkDescriptorImageInfo descriptorInfo{};
	#pragma endregion

};