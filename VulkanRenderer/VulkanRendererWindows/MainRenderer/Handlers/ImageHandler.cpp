#include "ImageHandler.h"
#include "../Structs/Texture.h"

#include <fstream>



ImageHandler::ImageHandler(VulkanDevice& r_VulkanDevice)
    : rm_VulkanDevice(r_VulkanDevice)
{}

ImageHandler::~ImageHandler()
{}

void ImageHandler::CreateImage(VkImage& r_Image, VkDeviceMemory& r_ImageMemory,
    uint32_t a_Width, uint32_t a_Height, VkFormat a_Format, VkImageTiling a_Tiling, VkImageUsageFlags a_Usage, VkMemoryPropertyFlags a_Properties)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(a_Width);
    imageInfo.extent.height = static_cast<uint32_t>(a_Height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = a_Format;
    imageInfo.tiling = a_Tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = a_Usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    if (vkCreateImage(rm_VulkanDevice, &imageInfo, nullptr, &r_Image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(rm_VulkanDevice, r_Image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = rm_VulkanDevice.FindMemoryType(memRequirements.memoryTypeBits, a_Properties);

    if (vkAllocateMemory(rm_VulkanDevice, &allocInfo, nullptr, &r_ImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(rm_VulkanDevice, r_Image, r_ImageMemory, 0);
}

void ImageHandler::CreateTextureImage(Texture& a_Texture, const unsigned char* a_Image)
{
    VkDeviceSize t_ImageSize = static_cast<VkDeviceSize>(a_Texture.texWidth * a_Texture.texHeight * RGBASize);

    if (!a_Image)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer t_StagingBuffer;
    VkDeviceMemory t_StagingBufferMemory;

    rm_VulkanDevice.CreateBuffer(t_ImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        t_StagingBuffer, t_StagingBufferMemory);

    void* data;
    vkMapMemory(rm_VulkanDevice, t_StagingBufferMemory, 0, t_ImageSize, 0, &data);
    memcpy(data, a_Image, static_cast<size_t>(t_ImageSize));
    vkUnmapMemory(rm_VulkanDevice, t_StagingBufferMemory);

    CreateImage(a_Texture.textureImage, a_Texture.textureImageMemory, a_Texture.texWidth, a_Texture.texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    TransitionImageLayout(a_Texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(t_StagingBuffer, a_Texture.textureImage, a_Texture.texWidth, a_Texture.texHeight);

    TransitionImageLayout(a_Texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(rm_VulkanDevice, t_StagingBuffer, nullptr);
    vkFreeMemory(rm_VulkanDevice, t_StagingBufferMemory, nullptr);
}

void ImageHandler::CopyBufferToImage(VkBuffer a_Buffer, VkImage a_Image, uint32_t a_Width, uint32_t a_Height)
{
    VkCommandBuffer t_CommandBuffer = rm_VulkanDevice.BeginSingleTimeCommands();

    VkBufferImageCopy t_Region{};
    t_Region.bufferOffset = 0;
    t_Region.bufferRowLength = 0;
    t_Region.bufferImageHeight = 0;

    t_Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    t_Region.imageSubresource.mipLevel = 0;
    t_Region.imageSubresource.baseArrayLayer = 0;
    t_Region.imageSubresource.layerCount = 1;

    t_Region.imageOffset = { 0, 0, 0 };
    t_Region.imageExtent = {
        a_Width,
        a_Height,
        1
    };

    vkCmdCopyBufferToImage(t_CommandBuffer,
        a_Buffer,
        a_Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &t_Region
    );

    rm_VulkanDevice.EndSingleTimeCommands(t_CommandBuffer);
}

void ImageHandler::TransitionImageLayout(VkImage a_Image, VkFormat a_Format, VkImageLayout a_OldLayout, VkImageLayout a_NewLayout)
{
    VkCommandBuffer t_CommandBuffer = rm_VulkanDevice.BeginSingleTimeCommands();

    VkImageMemoryBarrier t_Barrier{};
    t_Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    t_Barrier.oldLayout = a_OldLayout;
    t_Barrier.newLayout = a_NewLayout;

    t_Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    t_Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    t_Barrier.image = a_Image;
    t_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    t_Barrier.subresourceRange.baseMipLevel = 0;
    t_Barrier.subresourceRange.levelCount = 1;
    t_Barrier.subresourceRange.baseArrayLayer = 0;
    t_Barrier.subresourceRange.layerCount = 1;

    //Pipeline Layout setting.
    VkPipelineStageFlags t_SourceStage;
    VkPipelineStageFlags t_DestinationStage;

    if (a_NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        t_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (HasStencilComponent(a_Format)) {
            t_Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        t_Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (a_OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && a_NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
    {
        t_Barrier.srcAccessMask = 0;
        t_Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        t_SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        t_DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (a_OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && a_NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
        t_Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        t_Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        t_SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        t_DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (a_OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && a_NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
    {
        t_Barrier.srcAccessMask = 0;
        t_Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        t_SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        t_DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else 
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        t_CommandBuffer,
        t_SourceStage, t_DestinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &t_Barrier
    );

    rm_VulkanDevice.EndSingleTimeCommands(t_CommandBuffer);
}

VkImageView ImageHandler::CreateImageView(VkImage a_Image, VkFormat a_Format, VkImageAspectFlags a_AspectFlags)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = a_Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = a_Format;
	viewInfo.subresourceRange.aspectMask = a_AspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView t_ImageView;
	if (vkCreateImageView(rm_VulkanDevice, &viewInfo, nullptr, &t_ImageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return t_ImageView;
}

VkSampler ImageHandler::CreateTextureSampler()
{
	VkSamplerCreateInfo t_SamplerInfo{};
	t_SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	t_SamplerInfo.magFilter = VK_FILTER_LINEAR;
	t_SamplerInfo.minFilter = VK_FILTER_LINEAR;

	t_SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	t_SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	t_SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	t_SamplerInfo.anisotropyEnable = VK_TRUE;
	t_SamplerInfo.maxAnisotropy = rm_VulkanDevice.AnisotropyFilteringMax;

	t_SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	t_SamplerInfo.unnormalizedCoordinates = VK_FALSE;

	t_SamplerInfo.compareEnable = VK_FALSE;
	t_SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	t_SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	t_SamplerInfo.mipLodBias = 0.0f;
	t_SamplerInfo.minLod = 0.0f;
	t_SamplerInfo.maxLod = 0.0f;

	VkSampler t_Sampler;
	if (vkCreateSampler(rm_VulkanDevice, &t_SamplerInfo, nullptr, &t_Sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture sampler!");
	}
	return t_Sampler;
}

bool ImageHandler::HasStencilComponent(VkFormat a_Format)
{
    return a_Format == VK_FORMAT_D32_SFLOAT_S8_UINT || a_Format == VK_FORMAT_D24_UNORM_S8_UINT;
}