#pragma once
#include <stdint.h>

#include "Scene.h"

class Renderer;
class ObjectManager;
class CameraController;
class RenderFactory;
class GUISystem;

class Transform;
class Material;

namespace Engine 
{
	class SceneManager
	{
	public:
		SceneManager();
		~SceneManager();

		void UpdateScene(float a_Dt);

		void NewScene(const char* a_SceneName, SceneData a_SceneData, bool a_SetScene);
		void SetScene(uint32_t a_SceneID);

		void CreateShape(Transform* a_Transform, Material& a_Material);

	private:
		int32_t CurrentScene = -1;
		std::vector<Scene> m_Scenes;

		Renderer* m_Renderer;
		GUISystem* m_GuiSystem;

		ObjectManager* m_ObjectManager;
		CameraController* m_CameraController;

		RenderFactory* m_RenderFactory;
	};
}