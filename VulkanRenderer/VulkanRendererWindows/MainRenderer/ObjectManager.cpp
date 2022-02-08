#include "ObjectManager.h"
#include "Tools/VulkanInitializers.h"
#include <VulkanEngine/Framework/ResourceSystem/ResourceAllocator.h>
#include <VulkanEngine/Framework/ResourceSystem/MeshResource.h>


#include "RenderObjects/RenderShape.h"

ObjectManager::ObjectManager(Renderer* a_Renderer)
	: p_Renderer(a_Renderer)
{
	m_GeometryFactory.Init(p_Renderer);

	p_Renderer->SetRenderObjectsVector(&m_RenderObjects);
}

ObjectManager::~ObjectManager()
{
	for (size_t i = 0; i < m_RenderObjects.size(); i++)
	{
		//m_RenderObjects[i]->GetMeshData()->DeleteBuffers(p_Renderer->GetVulkanDevice().m_LogicalDevice);
		delete m_RenderObjects[i];
	}
	m_RenderObjects.clear();

	m_GeometryFactory.Cleanup(p_Renderer->GetVulkanDevice().m_LogicalDevice);
}

void ObjectManager::UpdateObjects(float a_Dt)
{
	(void)a_Dt;
	uint32_t t_ImageIndex;

	for (size_t i = 0; i < m_RenderObjects.size(); i++)
	{
		m_RenderObjects[i]->Update();
	}

	p_Renderer->DrawFrame(t_ImageIndex);
}

BaseRenderObject* ObjectManager::CreateRenderObject(Transform* a_Transform, Material& a_Material, GeometryType a_Type)
{
	RenderShape* t_Shape = new RenderShape(GetNextRenderID(), a_Transform, a_Material, 
		m_GeometryFactory.GetShape(a_Type));

	m_RenderObjects.push_back(t_Shape);

	return t_Shape;
}

BaseRenderObject* ObjectManager::CreateRenderObject(Transform* a_Transform, Material& a_Material, const char* a_MeshPath)
{
	RenderShape* t_Shape = new RenderShape(GetNextRenderID(), a_Transform, a_Material, 
		&Engine::ResourceAllocator::GetInstance().GetResource<Engine::Resource::MeshResource>(a_MeshPath, Engine::Resource::ResourceType::Mesh).meshData);

	m_RenderObjects.push_back(t_Shape);

	return t_Shape;
}