#pragma once
#include "Structs/PipelineData.h"
#include "DataStructures/ObjectPool.h"

#include "Handlers/DescriptorHandler.h"

#include "Structs/Shader.h"
#include "VulkanDevice.h"

class ShaderManager
{
public:
	ShaderManager(VulkanDevice& r_VulkanDevice, const VkExtent2D& r_VKSwapChainExtent);
	~ShaderManager();

	uint32_t CreatePipelineData(const Shader a_VertShader, const Shader a_FragShader, const VkRenderPass& r_RenderPass, std::vector<VkDescriptorSetLayout>& a_DescriptorSetLayouts);

	////Recreation needs to be done from the ground up again, these are just here as a reminder.
	//void RecreatePipelines(const VkRenderPass& r_RenderPass);
	//void RecreateDescriptors(const size_t a_FrameAmount, std::vector<VkBuffer>& r_UniformBuffers, VkDeviceSize a_BufferSize);

public:
	ObjectPool<PipeLineData, uint32_t> PipelinePool;

private:
	VulkanDevice& rm_VulkanDevice;
	const VkExtent2D& rmvk_SwapChainExtent;
};