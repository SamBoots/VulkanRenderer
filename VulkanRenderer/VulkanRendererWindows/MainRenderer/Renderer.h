#pragma once

#include "Window.h"

#include "ShaderManager.h"
#include "VulkanDebugger/VulkanDebug.h"
#include "RenderObjects/CameraObject.h"

#include "Structs/FrameData.h"
#include "VulkanSwapChain.h"

#include "RenderObjects/Components/MeshData.h"
#include "RenderObjects/Components/Material.h"

//Handlers
#include "Handlers/ImageHandler.h"
#include "Handlers/DepthHandler.h"
#include "RenderObjects/Geometry/GeometryFactory.h"

#include "RenderObjects/RenderObject.h"

constexpr uint32_t FRAMEBUFFER_AMOUNT = 2;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct Texture;
struct Shader;
namespace GUI{ class GUISystem; }


class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Init();

	//Setting up that is done later.
	void CreateStartBuffers();
	void SetupHandlers();

	void CleanupSwapChain();
	void RecreateSwapChain();

	void CreateVKInstance();
	VkPhysicalDevice PickPhysicalDevice();
	void CreateRenderPass();

	void CreateFrameBuffers();
	void CreateCommandPool();

	void CreateDepthResources();

	//BufferObject Creation.
	void CreateSyncObjects();

	//Set a new camera which the buffers get the viewprojection from.
	void ReplaceActiveCamera(CameraObject* a_Cam);

	//BufferData
	MeshHandle GenerateMesh(const std::vector<Vertex>& a_Vertices, const std::vector<uint32_t>& a_Indices);
	void SetupImage(Texture& a_Texture, const unsigned char* a_ImageBuffer);
	ShaderHandle CreateShader(const unsigned char* a_ShaderCode, const size_t a_CodeSize);
	MaterialHandle CreateMaterial(ShaderHandle a_Vert, ShaderHandle a_Frag,
		uint32_t a_UniCount, VkBuffer* a_UniBuffers,
		uint32_t a_ImageCount, Texture* a_Images,
		glm::vec4 a_Color = glm::vec4(0, 0, 0, 1));

	RenderObject CreateRenderObject(Engine::Transform* a_Transform, MaterialHandle a_MaterialHandle, MeshHandle a_MeshHandle);
	RenderObject CreateRenderObject(Engine::Transform* a_Transform, GeometryType a_Type, MaterialHandle a_MaterialHandle = RENDER_NULL_HANDLE);
	void SetupGUIsystem(GUI::GUISystem* p_GuiSystem);
	void SetupGeometryFactory();

	//Only handles single images at the moment.
	void CreateGlobalDescriptor();

	void DrawFrame();
	void DrawObjects(VkCommandBuffer a_CmdBuffer);
	void UpdateUniformBuffer(uint32_t a_CurrentImage);

	bool CheckDeviceExtensionSupport(VkPhysicalDevice a_Device);

	//Getters
	GLFWwindow* GetWindow() const { return m_Window->GetWindow(); }

	const float GetAspectRatio() { return m_VulkanSwapChain.SwapChainExtent.width / static_cast<float>(m_VulkanSwapChain.SwapChainExtent.height); }

	std::vector<VkBuffer> mvk_ViewProjectionBuffers;
	std::vector<VkDeviceMemory> mvk_ViewProjectionBuffersMemory;

private:
	bool IsDeviceSuitable(VkPhysicalDevice a_Device);

	DescriptorAllocator* m_DescriptorAllocator = nullptr;
	DescriptorLayoutCache* m_DescriptorLayoutCache = nullptr;

	VkDescriptorSetLayout m_GlobalSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet m_GlobalSet[2];

	//All the renderObjects in ObjectManager.
	std::vector<RenderObject> m_RenderObjects;
	GeometryFactory m_GeometryFactory{};


	ObjectPool<Material, MaterialHandle> m_MaterialPool;
	ObjectPool<MeshData, MeshHandle> m_MeshPool;
	ObjectPool<Shader, ShaderHandle> m_ShaderPool;

	//Window Data.
	Window* m_Window = nullptr;

	//Handlers
	ImageHandler* m_ImageHandler = nullptr;
	DepthHandler* m_DepthHandler = nullptr;

	//ShaderData
	ShaderManager* m_ShaderManager = nullptr;


	CameraObject* p_ActiveCamera = nullptr;

	std::vector<FrameData> m_FrameData;

	int m_CurrentFrame = 0;
	int m_PreviousFrame = -1;

	//Primary Vulkan Data;
	VkInstance mvk_Instance = VK_NULL_HANDLE;;

	VulkanDevice m_VulkanDevice;
	VulkanSwapChain m_VulkanSwapChain;

	VkQueue mvk_GraphicsQueue = VK_NULL_HANDLE;;
	VkQueue mvk_PresentQueue = VK_NULL_HANDLE;;

	//The RenderPipeline.
	VkRenderPass mvk_RenderPass = VK_NULL_HANDLE;;

	const std::vector<const char*> m_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	//DEBUGGING
	VulkanDebug* DE_VulkanDebug = nullptr;
#ifdef NDEBUG
	const bool DE_EnableValidationLayers = false;
#else
	const bool DE_EnableValidationLayers = true;
#endif
};
