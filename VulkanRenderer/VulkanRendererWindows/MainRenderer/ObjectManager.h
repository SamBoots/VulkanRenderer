#pragma once

#include "Renderer.h"
#include "RenderObjects/RenderFactories/RenderFactory.h"

class ObjectManager
{
public:
	ObjectManager(Renderer* a_Renderer);
	~ObjectManager();

	void SetupStartObjects();

	void UpdateObjects(float a_Dt);
	void AddRenderObject(BaseRenderObject* a_NewShape);

	void SetupDescriptor(uint32_t& a_DesID, uint32_t a_BufferCount, std::vector<TextureData>& a_Textures);

	//Increases the m_CurrentRenderID by 1 and returns it.
	size_t GetNextRenderID() { return m_CurrentRenderID++; }

private:
	size_t m_CurrentRenderID = 0;

	std::vector<BaseRenderObject*> m_RenderObjects;

	std::vector<TextureData> m_Textures;

public:
	//Data pipeline
	uint32_t pip_Pavillion;
	uint32_t pip_Triangle;

	//Data descriptor
	uint32_t des_Global;
	uint32_t des_Pavillion;
	uint32_t des_Triangle;

	Renderer* p_Renderer;
};