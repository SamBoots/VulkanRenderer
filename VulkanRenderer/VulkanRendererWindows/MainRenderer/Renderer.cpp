#define VK_USE_PLATFORM_WIN32_KHR
#include "Renderer.h"

#include <iostream>
#include <set>
#include <cstdint>
#include <algorithm>

#pragma warning (push, 0)
#include "ImGUI/imgui.h"

#include "imGUI/Backends/imgui_impl_vulkan.h"
#pragma warning (pop)


Renderer::Renderer()
{
	m_FrameData.resize(FRAMEBUFFER_AMOUNT);
	//Setting up GLFW.
	m_Window = new Window(800, 600, "VulkanTest");

	if (DE_EnableValidationLayers)
		DE_VulkanDebug = new VulkanDebug(mvk_Instance);

	CreateVKInstance();
	
	if (DE_EnableValidationLayers)
		DE_VulkanDebug->SetupDebugMessanger(nullptr);

	m_Window->SetupVKWindow(mvk_Instance);
	VkPhysicalDevice physicalDevice = PickPhysicalDevice();

	m_VulkanDevice.VulkanDeviceSetup(physicalDevice, DE_VulkanDebug, FRAMEBUFFER_AMOUNT);
	m_VulkanDevice.CreateLogicalDevice(std::vector<const char*>(), *m_Window, mvk_GraphicsQueue, mvk_PresentQueue);
	
	SetupHandlers();
	m_VulkanSwapChain.ConnectVulkanDevice(&m_VulkanDevice);
	m_VulkanSwapChain.CreateSwapChain(*m_Window, static_cast<uint32_t>(m_FrameData.size()));
	m_VulkanSwapChain.CreateImageViews(m_ImageHandler);
	CreateRenderPass();

	m_ShaderManager = new ShaderManager(m_VulkanDevice, m_VulkanSwapChain.SwapChainExtent);
}

Renderer::~Renderer()
{
	for (size_t i = 0; i < m_FrameData.size(); i++) {
		vkDestroySemaphore(m_VulkanDevice, m_FrameData[i].RenderFinishedSemaphore, nullptr);
		vkDestroySemaphore(m_VulkanDevice, m_FrameData[i].ImageAvailableSemaphore, nullptr);
		vkDestroyFence(m_VulkanDevice, m_FrameData[i].InFlightFence, nullptr);
	}
	//delete m_CommandHandler;

	delete m_ShaderManager;
	m_VulkanDevice.CleanupDevice();

	if (DE_EnableValidationLayers)
		delete DE_VulkanDebug;

	vkDestroySurfaceKHR(mvk_Instance, m_Window->GetSurface(), nullptr);
	vkDestroyInstance(mvk_Instance, nullptr);
	delete m_Window;
}

//Must be done after setting up the physical device.
void Renderer::SetupHandlers()
{
	m_ImageHandler = new ImageHandler(m_VulkanDevice);

	m_DepthHandler = new DepthHandler(m_VulkanDevice, m_ImageHandler);
}

//Call this after the creation of Vulkan.
void Renderer::SetupRenderObjects()
{
	mvk_ViewProjectionBuffers.resize(FRAMEBUFFER_AMOUNT);
	mvk_ViewProjectionBuffersMemory.resize(FRAMEBUFFER_AMOUNT);

	for (uint32_t i = 0; i < FRAMEBUFFER_AMOUNT; i++)
	{
		m_VulkanDevice.CreateUniformBuffers(mvk_ViewProjectionBuffers[i], mvk_ViewProjectionBuffersMemory[i], sizeof(ViewProjection));
	}

	CreateSyncObjects();
}

void Renderer::CleanupSwapChain()
{
	vkDeviceWaitIdle(m_VulkanDevice);
	for (size_t i = 0; i < m_FrameData.size(); i++) {
		vkDestroyFramebuffer(m_VulkanDevice, m_VulkanSwapChain.SwapChainFrameBuffers[i], nullptr);
	}

	//m_VulkanDevice.FreeCommandPool();

	vkDestroyPipeline(m_VulkanDevice, mvk_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_VulkanDevice, mvk_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_VulkanDevice, mvk_RenderPass, nullptr);

	for (size_t i = 0; i < m_FrameData.size(); i++)
	{
		vkDestroyBuffer(m_VulkanDevice, mvk_ViewProjectionBuffers[i], nullptr);
		vkFreeMemory(m_VulkanDevice, mvk_ViewProjectionBuffersMemory[i], nullptr);
	}
	
	m_DepthHandler->CleanupDepthTest();
}


void Renderer::RecreateSwapChain()
{
	m_Window->CheckMinimized();

	vkDeviceWaitIdle(m_VulkanDevice);
		
	CleanupSwapChain();

	m_VulkanSwapChain.CreateSwapChain(*m_Window, static_cast<uint32_t>(m_FrameData.size()));
	m_VulkanSwapChain.CreateImageViews(m_ImageHandler);
	CreateRenderPass();

	//Can be faulty, needs testing.
	//m_ShaderManager->RecreatePipelines(mvk_RenderPass);
	CreateDepthResources();
	CreateFrameBuffers();

	SetupRenderObjects();
	//m_ShaderManager->RecreateDescriptors(FRAMEBUFFER_AMOUNT, mvk_ViewProjectionBuffers, sizeof(ViewProjection));
}

void Renderer::CreateVKInstance()
{
	//If debug is enabled check if the layers are supported.
	if (DE_EnableValidationLayers && !DE_VulkanDebug->CheckValidationLayerSupport())
		throw std::runtime_error("validation layers requested, but not available!");


	//Setting up Vulkan: ApplicationInfo.
	VkApplicationInfo t_AppInfo{};
	t_AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	t_AppInfo.pApplicationName = "Zeep's VulkanTest";
	t_AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	t_AppInfo.pEngineName = "No Engine";
	t_AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	t_AppInfo.apiVersion = VK_API_VERSION_1_0;

	//Setting up Vulkan: VKInstance Setup
	VkInstanceCreateInfo t_CreateInfo{};
	t_CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	t_CreateInfo.pApplicationInfo = &t_AppInfo;

	auto t_Extentions = m_Window->GetRequiredExtentions(DE_EnableValidationLayers);

	t_CreateInfo.enabledExtensionCount = static_cast<uint32_t>(t_Extentions.size());
	t_CreateInfo.ppEnabledExtensionNames = t_Extentions.data();

	//Set the Debug Layers if debug is enabled, otherwise set it to 0
	if (DE_EnableValidationLayers)
	{
		DE_VulkanDebug->AddDebugLayerInstance(t_CreateInfo);

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		DE_VulkanDebug->PopulateDebugStartupMessengerCreateInfo(debugCreateInfo);
		t_CreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}
	else
	{
		t_CreateInfo.enabledLayerCount = 0;
		t_CreateInfo.pNext = nullptr;
	}

	//Create and check if the Instance is a success.
	if (vkCreateInstance(&t_CreateInfo, nullptr, &mvk_Instance) != VK_SUCCESS)
	{
		throw printf("Vulkan not successfully Instanced.");
	}
}

VkPhysicalDevice Renderer::PickPhysicalDevice()
{
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	uint32_t t_DeviceCount = 0;
	vkEnumeratePhysicalDevices(mvk_Instance, &t_DeviceCount, nullptr);

	//No devices that support Vulcan.
	if (t_DeviceCount == 0) 
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> t_Devices(t_DeviceCount);
	vkEnumeratePhysicalDevices(mvk_Instance, &t_DeviceCount, t_Devices.data());

	for (const VkPhysicalDevice& device : t_Devices) 
	{
		if (IsDeviceSuitable(device)) 
		{
			VkPhysicalDeviceProperties t_PhysDeviceProperties;
			vkGetPhysicalDeviceProperties(device, &t_PhysDeviceProperties);
			if (t_PhysDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				physicalDevice = device;
				break;
			}
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	return physicalDevice;
}

void Renderer::CreateRenderPass()
{
	//Color Attachment.
	VkAttachmentDescription t_ColorAttachment{};
	t_ColorAttachment.format = m_VulkanSwapChain.SwapChainImageFormat;
	t_ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	t_ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	t_ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	t_ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	t_ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	t_ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	t_ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//t_ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference t_ColorAttachmentRef{};
	t_ColorAttachmentRef.attachment = 0;
	t_ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//DEPTH BUFFER
	VkAttachmentDescription t_DepthAttachment{};
	t_DepthAttachment.format = m_DepthHandler->FindDepthFormat();
	t_DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	t_DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	t_DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	t_DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	t_DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	t_DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	t_DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference t_DepthAttachmentRef{};
	t_DepthAttachmentRef.attachment = 1;
	t_DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription t_Subpass{};
	t_Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	t_Subpass.colorAttachmentCount = 1;
	t_Subpass.pColorAttachments = &t_ColorAttachmentRef;
	t_Subpass.pDepthStencilAttachment = &t_DepthAttachmentRef;

	VkSubpassDependency t_Dependency{};
	t_Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	t_Dependency.dstSubpass = 0;

	t_Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;;
	t_Dependency.srcAccessMask = 0;

	t_Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;;
	t_Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;

	//Setting the struct.
	std::array<VkAttachmentDescription, 2> t_Attachments = { t_ColorAttachment, t_DepthAttachment };
	VkRenderPassCreateInfo t_RenderPassInfo{};
	t_RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	t_RenderPassInfo.attachmentCount = static_cast<uint32_t>(t_Attachments.size());;
	t_RenderPassInfo.pAttachments = t_Attachments.data();
	t_RenderPassInfo.subpassCount = 1;
	t_RenderPassInfo.pSubpasses = &t_Subpass;
	t_RenderPassInfo.dependencyCount = 1;
	t_RenderPassInfo.pDependencies = &t_Dependency;

	if (vkCreateRenderPass(m_VulkanDevice, &t_RenderPassInfo, nullptr, &mvk_RenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

uint32_t Renderer::CreateGraphicsPipeline(std::vector<VkDescriptorSetLayout>& a_DescriptorSetLayouts)
{
	return m_ShaderManager->CreatePipelineData(mvk_RenderPass, a_DescriptorSetLayouts);
}

void Renderer::CreateFrameBuffers()
{
	for (size_t i = 0; i < m_FrameData.size(); i++)
	{
		std::array<VkImageView, 2> t_Attachments = {
		m_VulkanSwapChain.SwapChainImageViews[i],
		m_DepthHandler->GetDepthTest().depthImageView
		};

		VkFramebufferCreateInfo t_FramebufferInfo{};
		t_FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		t_FramebufferInfo.renderPass = mvk_RenderPass;
		t_FramebufferInfo.attachmentCount = static_cast<uint32_t>(t_Attachments.size());
		t_FramebufferInfo.pAttachments = t_Attachments.data();
		t_FramebufferInfo.width = m_VulkanSwapChain.SwapChainExtent.width;
		t_FramebufferInfo.height = m_VulkanSwapChain.SwapChainExtent.height;
		t_FramebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_VulkanDevice, &t_FramebufferInfo, nullptr, &m_VulkanSwapChain.SwapChainFrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void Renderer::CreateCommandPool()
{
	m_VulkanDevice.CreateCommandPools(FindQueueFamilies(m_VulkanDevice.m_PhysicalDevice, *m_Window));
}

void Renderer::CreateDepthResources()
{
	m_DepthHandler->CreateDepthResources(m_VulkanSwapChain.SwapChainExtent.width, m_VulkanSwapChain.SwapChainExtent.height);
}

void Renderer::CreateSyncObjects()
{
	//mvk_ImageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	//mvk_RenderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	//mvk_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	//mvk_ImagesInFlight.resize(mvk_SwapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo t_SemaphoreInfo{};
	t_SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo t_FenceInfo{};
	t_FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	t_FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < m_FrameData.size(); i++)
	{
		if (vkCreateSemaphore(m_VulkanDevice, &t_SemaphoreInfo, nullptr, &m_FrameData[i].ImageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_VulkanDevice, &t_SemaphoreInfo, nullptr, &m_FrameData[i].RenderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(m_VulkanDevice, &t_FenceInfo, nullptr, &m_FrameData[i].InFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores for a frame!");
		}
	}
}

void Renderer::ReplaceActiveCamera(CameraObject* a_Cam)
{
	p_ActiveCamera = a_Cam;
}

void Renderer::SetupMesh(MeshData* a_MeshData)
{
	m_VulkanDevice.CreateVertexBuffers(a_MeshData->GetVertexData());
	m_VulkanDevice.CreateIndexBuffers(a_MeshData->GetIndexData());
}

void Renderer::SetupImage(Texture& a_Texture)
{
	m_ImageHandler->CreateTextureImage(a_Texture);

	a_Texture.textureImageView = m_ImageHandler->CreateImageView(a_Texture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	a_Texture.textureSampler = m_ImageHandler->CreateTextureSampler();
}

void Renderer::DrawFrame(uint32_t& r_ImageIndex)
{
	FrameData& frameData = m_FrameData[m_CurrentFrame];

	vkWaitForFences(m_VulkanDevice, 1, &frameData.InFlightFence, VK_TRUE, UINT64_MAX);

	VkResult t_Result = vkAcquireNextImageKHR(m_VulkanDevice, m_VulkanSwapChain.SwapChain, UINT64_MAX, frameData.ImageAvailableSemaphore, VK_NULL_HANDLE, &r_ImageIndex);

	UpdateUniformBuffer(r_ImageIndex);

	if (t_Result == VK_ERROR_OUT_OF_DATE_KHR || t_Result == VK_SUBOPTIMAL_KHR || m_Window->m_FrameBufferResized)
	{
		m_Window->m_FrameBufferResized = false;
		RecreateSwapChain();
		return;
	}
	else if (t_Result != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (frameData.ImageInFlight != VK_NULL_HANDLE)
	{
		vkWaitForFences(m_VulkanDevice, 1, &frameData.ImageInFlight, VK_TRUE, UINT64_MAX);
	}
	// Mark the image as now being in use by this frame
	frameData.ImageInFlight = frameData.InFlightFence;

	//Draw the objects using a single command buffer.
	m_VulkanDevice.ClearPreviousCommand(m_CurrentFrame);

	VkCommandBuffer& t_MainBuffer = m_VulkanDevice.CreateAndBeginCommand(m_CurrentFrame, FindQueueFamilies(m_VulkanDevice.m_PhysicalDevice, *m_Window).graphicsFamily.value(),
		mvk_RenderPass, m_VulkanSwapChain.SwapChainFrameBuffers[m_CurrentFrame], m_VulkanSwapChain.SwapChainExtent);

	DrawObjects(t_MainBuffer);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), t_MainBuffer);
	m_VulkanDevice.EndCommand(t_MainBuffer);


	VkSubmitInfo t_SubmitInfo{};
	t_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore t_WaitSemaphores[] = { frameData.ImageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	t_SubmitInfo.waitSemaphoreCount = 1;
	t_SubmitInfo.pWaitSemaphores = t_WaitSemaphores;
	t_SubmitInfo.pWaitDstStageMask = waitStages;

	t_SubmitInfo.commandBufferCount = 1;
	t_SubmitInfo.pCommandBuffers = &t_MainBuffer;

	VkSemaphore t_SignalSemaphores[] = { frameData.RenderFinishedSemaphore };
	t_SubmitInfo.signalSemaphoreCount = 1;
	t_SubmitInfo.pSignalSemaphores = t_SignalSemaphores;



	vkResetFences(m_VulkanDevice, 1, &frameData.InFlightFence);
	if (vkQueueSubmit(mvk_GraphicsQueue, 1, &t_SubmitInfo, frameData.InFlightFence) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR t_PresentInfo{};
	t_PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	t_PresentInfo.waitSemaphoreCount = 1;
	t_PresentInfo.pWaitSemaphores = t_SignalSemaphores;

	VkSwapchainKHR t_SwapChains[] = { m_VulkanSwapChain.SwapChain };
	t_PresentInfo.swapchainCount = 1;
	t_PresentInfo.pSwapchains = t_SwapChains;
	t_PresentInfo.pImageIndices = &r_ImageIndex;

	t_PresentInfo.pResults = nullptr; // Optional

	t_Result = vkQueuePresentKHR(mvk_PresentQueue, &t_PresentInfo);

	if (t_Result == VK_ERROR_OUT_OF_DATE_KHR || t_Result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapChain();
	}
	else if (t_Result != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	//vkQueueWaitIdle(m_VKPresentQueue);
	m_PreviousFrame = m_CurrentFrame;
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::DrawObjects(VkCommandBuffer& r_CmdBuffer)
{
	//Performance boost when getting the same data needed.
	MeshData* t_LastMeshData = nullptr;
	Material* t_LastMaterial = nullptr;

	uint32_t t_LastPipelineID = UINT32_MAX;
	PipeLineData* t_CurrentPipeLine = nullptr;

	//Create this once for performance boost of not creating a mat4 everytime.
	InstanceModel t_PushInstance{};

	for (size_t i = 0; i < p_RenderObjects->size(); i++)
	{
		BaseRenderObject* t_RenderObject = p_RenderObjects->at(i);
		uint32_t t_PipelineID = t_RenderObject->GetPipeLineID();

		//only bind the pipeline if it doesn't match with the already bound one
		if (t_PipelineID != t_LastPipelineID)
		{
			t_CurrentPipeLine = &m_ShaderManager->PipelinePool.Get(t_PipelineID);
			t_LastPipelineID = t_PipelineID;

			vkCmdBindPipeline(r_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, t_CurrentPipeLine->pipeLine);

			vkCmdBindDescriptorSets(r_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, t_CurrentPipeLine->pipeLineLayout, 0, 1, &GlobalSet[m_CurrentFrame], 0, nullptr);
			vkCmdBindDescriptorSets(r_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, t_CurrentPipeLine->pipeLineLayout, 1, 1, &t_RenderObject->GetMaterialDescriptorSet(), 0, nullptr);
		}
		else if (t_RenderObject->GetMaterial() != t_LastMaterial)
		{
			vkCmdBindDescriptorSets(r_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, t_CurrentPipeLine->pipeLineLayout, 1, 1, &t_RenderObject->GetMaterialDescriptorSet(), 0, nullptr);
		}

		//Set the Push constant data.
		t_PushInstance.model = t_RenderObject->GetModelMatrix();

		vkCmdPushConstants(r_CmdBuffer, t_CurrentPipeLine->pipeLineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(InstanceModel), &t_PushInstance);

		//only bind the mesh if it's a different one from last bind
		if (t_RenderObject->GetMeshData() != t_LastMeshData) {
			//bind the mesh vertex buffer with offset 0
			VkDeviceSize t_VertOffset = 0;
			//Set Vertex
			vkCmdBindVertexBuffers(r_CmdBuffer, 0, 1, &t_RenderObject->GetVertexData()->GetBuffer(), &t_VertOffset);

			//Set Indices
			vkCmdBindIndexBuffer(r_CmdBuffer, t_RenderObject->GetIndexData()->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

			t_LastMeshData = t_RenderObject->GetMeshData();
		}

		//vkCmdDraw(r_CmdBuffer, static_cast<uint32_t>(t_RenderObject->GetVertexData()->GetElementCount()), 1, 0, 0);
		vkCmdDrawIndexed(r_CmdBuffer, static_cast<uint32_t>(t_RenderObject->GetIndexData()->GetElementCount()), 1, 0, 0, 0);
	}
}

void Renderer::UpdateUniformBuffer(uint32_t a_CurrentImage)
{
	ViewProjection ubo = p_ActiveCamera->GetViewProjection();

	void* data;
	vkMapMemory(m_VulkanDevice, mvk_ViewProjectionBuffersMemory[a_CurrentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_VulkanDevice, mvk_ViewProjectionBuffersMemory[a_CurrentImage]);
}

bool Renderer::IsDeviceSuitable(VkPhysicalDevice a_Device)
{
	//Queues are supported.
	QueueFamilyIndices t_Indices = FindQueueFamilies(a_Device, *m_Window);

	//Extentions from m_DeviceExtensions are supported.
	bool t_ExtensionsSupported = CheckDeviceExtensionSupport(a_Device);

	//Swapchain is sufficiently supported.
	bool t_SwapChainAdequate = false;
	if (t_ExtensionsSupported) 
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(a_Device, *m_Window);
		t_SwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures t_SupportedFeatures;
	vkGetPhysicalDeviceFeatures(a_Device, &t_SupportedFeatures);

	//The device works with everything that the program needs.
	return t_Indices.isComplete() && t_ExtensionsSupported && t_SwapChainAdequate && t_SupportedFeatures.samplerAnisotropy;;
}

bool Renderer::CheckDeviceExtensionSupport(VkPhysicalDevice a_Device)
{
	uint32_t t_ExtensionCount;
	vkEnumerateDeviceExtensionProperties(a_Device, nullptr, &t_ExtensionCount, nullptr);

	std::vector<VkExtensionProperties> t_AvailableExtensions(t_ExtensionCount);
	vkEnumerateDeviceExtensionProperties(a_Device, nullptr, &t_ExtensionCount, t_AvailableExtensions.data());

	std::set<std::string> t_RequiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

	for (const auto& extension : t_AvailableExtensions) 
	{
		t_RequiredExtensions.erase(extension.extensionName);
	}

	return t_RequiredExtensions.empty();
}