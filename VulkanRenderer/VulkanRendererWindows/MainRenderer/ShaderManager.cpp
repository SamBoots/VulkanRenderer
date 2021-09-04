#include "ShaderManager.h"
#include <Structs/BufferData.h>

#include <iostream>

ShaderManager::ShaderManager(VulkanDevice& r_VulkanDevice, const VkExtent2D& r_VKSwapChainExtent)
	: rm_VulkanDevice(r_VulkanDevice), rmvk_SwapChainExtent(r_VKSwapChainExtent)
{}

ShaderManager::~ShaderManager()
{
	for (uint32_t i = 0; i < DescriptorPool.Size(); i++)
	{
		bool isFull;
		DescriptorData& descData = DescriptorPool.GetFilledObject(isFull, i);
		if (isFull)
			vkDestroyDescriptorSetLayout(rm_VulkanDevice, descData.descriptorLayout, nullptr);
	}
}

uint32_t ShaderManager::CreateDescriptorSetLayout(TextureData* a_TextureData)
{
	//Set desciptorData's ID. (Get more advanced later on.)
	uint32_t descriptorID;
	DescriptorData& descriptorData = DescriptorPool.GetEmptyObject(descriptorID);
	descriptorData.descID = descriptorID;

	descriptorData.texture = a_TextureData;

	VkDescriptorSetLayoutBinding t_UboLayoutBinding{};
	t_UboLayoutBinding.binding = 0;
	t_UboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	t_UboLayoutBinding.descriptorCount = 1;
	t_UboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	t_UboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding t_SamplerLayoutBinding{};
	t_SamplerLayoutBinding.binding = 1;
	t_SamplerLayoutBinding.descriptorCount = 1;
	t_SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	t_SamplerLayoutBinding.pImmutableSamplers = nullptr;
	t_SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> t_Bindings = { t_UboLayoutBinding, t_SamplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo t_LayoutInfo{};
	t_LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	t_LayoutInfo.bindingCount = static_cast<uint32_t>(t_Bindings.size());
	t_LayoutInfo.pBindings = t_Bindings.data();


	if (vkCreateDescriptorSetLayout(rm_VulkanDevice, &t_LayoutInfo, nullptr, &descriptorData.descriptorLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	return descriptorData.descID;
}

uint32_t ShaderManager::CreatePipelineData(const VkRenderPass& r_RenderPass, uint32_t a_DescriptorID)
{
	uint32_t pipelineID;

	PipeLineData& pipeLineData = PipelinePool.GetEmptyObject(pipelineID);
	pipeLineData.pipeID = pipelineID;
	pipeLineData.p_DescriptorData = &DescriptorPool.Get(a_DescriptorID);

	CreateGraphicsPipeline(r_RenderPass, pipeLineData);

	return pipeLineData.pipeID;
}

void ShaderManager::RecreatePipelines(const VkRenderPass& r_RenderPass)
{
	for (uint32_t i = 0; i < PipelinePool.Size(); i++)
	{
		bool isFull;
		PipeLineData& pipeData = PipelinePool.GetFilledObject(isFull, i);

		if (isFull)
			CreateGraphicsPipeline(r_RenderPass, pipeData);
	}
}

void ShaderManager::CreateDescriptorPool(const size_t a_FrameAmount, uint32_t a_DescID)
{
	std::array<VkDescriptorPoolSize, 2> t_PoolSizes{};
	t_PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	t_PoolSizes[0].descriptorCount = static_cast<uint32_t>(a_FrameAmount);
	t_PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	t_PoolSizes[1].descriptorCount = static_cast<uint32_t>(a_FrameAmount);

	VkDescriptorPoolCreateInfo t_PoolInfo{};
	t_PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	t_PoolInfo.poolSizeCount = static_cast<uint32_t>(t_PoolSizes.size());
	t_PoolInfo.pPoolSizes = t_PoolSizes.data();
	t_PoolInfo.maxSets = static_cast<uint32_t>(a_FrameAmount);

	if (vkCreateDescriptorPool(rm_VulkanDevice, &t_PoolInfo, nullptr, &DescriptorPool.Get(a_DescID).descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void ShaderManager::CreateDescriptorSet(const size_t a_FrameAmount, uint32_t a_DescID, std::vector<VkBuffer>& r_UniformBuffers, VkDeviceSize a_BufferSize)
{
	DescriptorData& r_DescData = DescriptorPool.Get(a_DescID);
	std::vector<VkDescriptorSetLayout> t_Layouts(a_FrameAmount, r_DescData.descriptorLayout);

	VkDescriptorSetAllocateInfo t_AllocInfo{};
	t_AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	t_AllocInfo.descriptorPool = r_DescData.descriptorPool;
	t_AllocInfo.descriptorSetCount = 2;//static_cast<uint32_t>(a_FrameAmount);
	t_AllocInfo.pSetLayouts = t_Layouts.data();

	std::vector<VkDescriptorSet> DescriptorSet;
	DescriptorSet.resize(2);

	if (vkAllocateDescriptorSets(rm_VulkanDevice, &t_AllocInfo, DescriptorSet.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < a_FrameAmount; i++)
	{
		//VkDescriptorBufferInfo t_BufferInfo{};
		//t_BufferInfo.buffer = r_UniformBuffers[i];
		//t_BufferInfo.offset = 0;
		//t_BufferInfo.range = a_BufferSize;

		VkDescriptorBufferInfo t_BufferInfo = CreateDescriptorBufferInfo(r_UniformBuffers[i], 0, a_BufferSize);

		//VkDescriptorImageInfo t_ImageInfo{};
		//t_ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//t_ImageInfo.imageView = r_DescData.texture->textureImageView;
		//t_ImageInfo.sampler = r_DescData.texture->textureSampler;

		VkDescriptorImageInfo t_ImageInfo = CreateDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, r_DescData.texture->textureImageView, r_DescData.texture->textureSampler);

		std::array<VkWriteDescriptorSet, 2> t_DescriptorWrites{};
		//Uniform Info
		t_DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		t_DescriptorWrites[0].dstSet = DescriptorSet[i];
		t_DescriptorWrites[0].dstBinding = 0;
		t_DescriptorWrites[0].dstArrayElement = 0;
		t_DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		t_DescriptorWrites[0].descriptorCount = 1;
		t_DescriptorWrites[0].pBufferInfo = &t_BufferInfo;
		//Image Info
		t_DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		t_DescriptorWrites[1].dstSet = DescriptorSet[i];
		t_DescriptorWrites[1].dstBinding = 1;
		t_DescriptorWrites[1].dstArrayElement = 0;
		t_DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		t_DescriptorWrites[1].descriptorCount = 1;
		t_DescriptorWrites[1].pImageInfo = &t_ImageInfo;

		vkUpdateDescriptorSets(rm_VulkanDevice, static_cast<uint32_t>(t_DescriptorWrites.size()),
			t_DescriptorWrites.data(), 0, nullptr);

		r_DescData.descriptorSets[i].push_back(DescriptorSet[i]);
	}
}

void ShaderManager::RecreateDescriptors(const size_t a_FrameAmount, std::vector<VkBuffer>& r_UniformBuffers, VkDeviceSize a_BufferSize)
{
	for (size_t i = 0; i < DescriptorPool.Size(); i++)
	{
		bool isFull;
		DescriptorData& descData = DescriptorPool.GetFilledObject(isFull, i);

		if (isFull)
		{
			vkDestroyDescriptorPool(rm_VulkanDevice, descData.descriptorPool, nullptr);

			for (size_t i = 0; i < a_FrameAmount; i++)
			{
				descData.descriptorSets[i].clear();
			}

			CreateDescriptorPool(a_FrameAmount, i);
			CreateDescriptorSet(a_FrameAmount, i, r_UniformBuffers, a_BufferSize);
		}
	}
}

VkDescriptorBufferInfo ShaderManager::CreateDescriptorBufferInfo(VkBuffer& r_UniformBuffer, VkDeviceSize a_Offset, VkDeviceSize a_BufferSize)
{
	VkDescriptorBufferInfo t_BufferInfo{};
	t_BufferInfo.buffer = r_UniformBuffer;
	t_BufferInfo.offset = 0;
	t_BufferInfo.range = a_BufferSize;

	return t_BufferInfo;
}

VkDescriptorImageInfo ShaderManager::CreateDescriptorImageInfo(VkImageLayout a_ImageLayout, VkImageView& r_ImageView, VkSampler& r_Sampler)
{
	VkDescriptorImageInfo t_ImageInfo{};
	t_ImageInfo.imageLayout = a_ImageLayout;
	t_ImageInfo.imageView = r_ImageView;
	t_ImageInfo.sampler = r_Sampler;

	return t_ImageInfo;
}

void ShaderManager::CreateGraphicsPipeline(const VkRenderPass& r_RenderPass, PipeLineData& r_PipeLineData)
{
	auto t_VertShaderCode = ResourceLoader::ReadFile("../Resources/Shaders/unlitVert.spv");
	auto t_FragShaderCode = ResourceLoader::ReadFile("../Resources/Shaders/unlitFrag.spv");

	VkShaderModule t_VertShaderModule = CreateShaderModule(t_VertShaderCode);
	VkShaderModule t_FragShaderModule = CreateShaderModule(t_FragShaderCode);

	//Setting up Vertex.
	VkPipelineShaderStageCreateInfo t_VertShaderStageInfo{};
	t_VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	t_VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	t_VertShaderStageInfo.pSpecializationInfo = nullptr;
	t_VertShaderStageInfo.module = t_VertShaderModule;
	t_VertShaderStageInfo.pName = "main";

	//Setting up Fragment.
	VkPipelineShaderStageCreateInfo t_FragShaderStageInfo{};
	t_FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	t_FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	t_FragShaderStageInfo.pSpecializationInfo = nullptr;
	t_FragShaderStageInfo.module = t_FragShaderModule;
	t_FragShaderStageInfo.pName = "main";

	//These are hard coded.
	VkPipelineShaderStageCreateInfo t_ShaderStages[] = { t_VertShaderStageInfo, t_FragShaderStageInfo };

	auto t_BindingDescription = Vertex::GetBindingDescription();
	auto t_AttributeDescriptions = Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo t_VertexInputInfo{};
	t_VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	t_VertexInputInfo.vertexBindingDescriptionCount = 1;
	t_VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(t_AttributeDescriptions.size());
	t_VertexInputInfo.pVertexBindingDescriptions = &t_BindingDescription;
	t_VertexInputInfo.pVertexAttributeDescriptions = t_AttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo t_InputAssembly{};
	t_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	t_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	t_InputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport t_Viewport{};
	t_Viewport.x = 0.0f;
	t_Viewport.y = 0.0f;
	t_Viewport.width = (float)rmvk_SwapChainExtent.width;
	t_Viewport.height = (float)rmvk_SwapChainExtent.height;
	t_Viewport.minDepth = 0.0f;
	t_Viewport.maxDepth = 1.0f;

	VkRect2D t_Scissor{};
	t_Scissor.offset = { 0, 0 };
	t_Scissor.extent = rmvk_SwapChainExtent;


	VkPipelineViewportStateCreateInfo t_ViewportState{};
	t_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	t_ViewportState.viewportCount = 1;
	t_ViewportState.pViewports = &t_Viewport;
	t_ViewportState.scissorCount = 1;
	t_ViewportState.pScissors = &t_Scissor;

	VkPipelineRasterizationStateCreateInfo t_Rasterizer{};
	t_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	t_Rasterizer.depthClampEnable = VK_FALSE;

	t_Rasterizer.rasterizerDiscardEnable = VK_FALSE;
	t_Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	t_Rasterizer.lineWidth = 1.0f;

	t_Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	t_Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	t_Rasterizer.depthBiasEnable = VK_FALSE;
	t_Rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	t_Rasterizer.depthBiasClamp = 0.0f; // Optional
	t_Rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	//Anti-Aliasing Settingsm, it's disabled.
	VkPipelineMultisampleStateCreateInfo t_Multisampling{};
	t_Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	t_Multisampling.sampleShadingEnable = VK_FALSE;
	t_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	t_Multisampling.minSampleShading = 1.0f; // Optional
	t_Multisampling.pSampleMask = nullptr; // Optional
	t_Multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	t_Multisampling.alphaToOneEnable = VK_FALSE; // Optional

	//Empty/not used now.
	VkPipelineDepthStencilStateCreateInfo t_DepthStencil{};
	t_DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	t_DepthStencil.depthTestEnable = VK_TRUE;
	t_DepthStencil.depthWriteEnable = VK_TRUE;
	t_DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	t_DepthStencil.depthBoundsTestEnable = VK_FALSE;
	t_DepthStencil.minDepthBounds = 0.0f; // Optional
	t_DepthStencil.maxDepthBounds = 1.0f; // Optional
	t_DepthStencil.stencilTestEnable = VK_FALSE;
	t_DepthStencil.front = {}; // Optional
	t_DepthStencil.back = {}; // Optional


	VkPipelineColorBlendAttachmentState t_ColorBlendAttachment{};
	t_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	t_ColorBlendAttachment.blendEnable = VK_FALSE;
	t_ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	t_ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	t_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	t_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	t_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	t_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo t_ColorBlending{};
	t_ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	t_ColorBlending.logicOpEnable = VK_FALSE;
	t_ColorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	t_ColorBlending.attachmentCount = 1;
	t_ColorBlending.pAttachments = &t_ColorBlendAttachment;
	t_ColorBlending.blendConstants[0] = 0.0f; // Optional
	t_ColorBlending.blendConstants[1] = 0.0f; // Optional
	t_ColorBlending.blendConstants[2] = 0.0f; // Optional
	t_ColorBlending.blendConstants[3] = 0.0f; // Optional

	//If you go over 128 bytes (1 matrix is 64 bytes) then use UBO's.
	VkPushConstantRange t_InstanceModelPushConstantRange{};
	t_InstanceModelPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	t_InstanceModelPushConstantRange.offset = 0;
	t_InstanceModelPushConstantRange.size = sizeof(InstanceModel);

	//Generating the Actual Pipeline
	VkPipelineLayoutCreateInfo t_PipelineLayoutInfo{};
	t_PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	t_PipelineLayoutInfo.setLayoutCount = 1; // Optional
	t_PipelineLayoutInfo.pSetLayouts = &r_PipeLineData.p_DescriptorData->descriptorLayout; // Optional
	t_PipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
	t_PipelineLayoutInfo.pPushConstantRanges = &t_InstanceModelPushConstantRange; // Optional

	if (vkCreatePipelineLayout(rm_VulkanDevice, &t_PipelineLayoutInfo, nullptr, &r_PipeLineData.pipeLineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo t_PipelineInfo{};
	t_PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	t_PipelineInfo.stageCount = 2;
	t_PipelineInfo.pStages = t_ShaderStages;

	t_PipelineInfo.pVertexInputState = &t_VertexInputInfo;
	t_PipelineInfo.pInputAssemblyState = &t_InputAssembly;
	t_PipelineInfo.pViewportState = &t_ViewportState;
	t_PipelineInfo.pRasterizationState = &t_Rasterizer;
	t_PipelineInfo.pMultisampleState = &t_Multisampling;
	t_PipelineInfo.pDepthStencilState = &t_DepthStencil; // Optional
	t_PipelineInfo.pColorBlendState = &t_ColorBlending;
	t_PipelineInfo.pDynamicState = nullptr; // Optional


	t_PipelineInfo.layout = r_PipeLineData.pipeLineLayout;

	t_PipelineInfo.renderPass = r_RenderPass;
	t_PipelineInfo.subpass = 0;

	t_PipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	t_PipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(rm_VulkanDevice, VK_NULL_HANDLE, 1, &t_PipelineInfo, nullptr, &r_PipeLineData.pipeLine) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(rm_VulkanDevice, t_VertShaderModule, nullptr);
	vkDestroyShaderModule(rm_VulkanDevice, t_FragShaderModule, nullptr);
}

VkShaderModule ShaderManager::CreateShaderModule(const std::vector<char>& a_Code)
{
	VkShaderModuleCreateInfo t_CreateInfo{};
	t_CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	t_CreateInfo.codeSize = a_Code.size();
	t_CreateInfo.pCode = reinterpret_cast<const uint32_t*>(a_Code.data());

	VkShaderModule t_ShaderModule;
	if (vkCreateShaderModule(rm_VulkanDevice, &t_CreateInfo, nullptr, &t_ShaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return t_ShaderModule;
}