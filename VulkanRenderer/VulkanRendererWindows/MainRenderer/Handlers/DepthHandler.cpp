#include "DepthHandler.h"

#include "BufferHandler.h"
#include "ImageHandler.h"
#include <fstream>

DepthHandler::DepthHandler(VkDevice& r_Device, VkPhysicalDevice& r_PhysDevice, ImageHandler* a_ImageHandler)
	:	rm_Device(r_Device), rm_PhysDevice(r_PhysDevice)
{
	p_ImageHandler = a_ImageHandler;
}

DepthHandler::~DepthHandler()
{}

void DepthHandler::CleanupDepthTest()
{
	vkDestroyImageView(rm_Device, m_DepthTest.depthImageView, nullptr);
	vkDestroyImage(rm_Device, m_DepthTest.depthImage, nullptr);
	vkFreeMemory(rm_Device, m_DepthTest.depthImageMemory, nullptr);
}

void DepthHandler::CreateDepthResources(uint32_t a_Width, uint32_t a_Height)
{
	VkFormat t_DepthFormat = FindDepthFormat();

	p_ImageHandler->CreateImage(m_DepthTest.depthImage, m_DepthTest.depthImageMemory, a_Width, a_Height, t_DepthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_DepthTest.depthImageView = p_ImageHandler->CreateImageView(m_DepthTest.depthImage, t_DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);


	//endl.
	p_ImageHandler->TransitionImageLayout(m_DepthTest.depthImage, t_DepthFormat, 
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

VkFormat DepthHandler::FindSupportedFormat(const std::vector<VkFormat>& r_Candidates, VkImageTiling a_Tiling, VkFormatFeatureFlags a_Features)
{
	for (VkFormat t_Format : r_Candidates) 
	{
		VkFormatProperties t_Props;
		vkGetPhysicalDeviceFormatProperties(rm_PhysDevice, t_Format, &t_Props);

		if (a_Tiling == VK_IMAGE_TILING_LINEAR && (t_Props.linearTilingFeatures & a_Features) == a_Features) 
		{
			return t_Format;
		}
		else if (a_Tiling == VK_IMAGE_TILING_OPTIMAL && (t_Props.optimalTilingFeatures & a_Features) == a_Features) 
		{
			return t_Format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat DepthHandler::FindDepthFormat()
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

DepthTest& DepthHandler::GetDepthTest()
{
	return m_DepthTest;
}