#pragma once
#include <stdint.h>
#include <unordered_map>
//#include "Resource.h"
#include "TextureResource.h"


struct Model;
class Material;

class Renderer;

namespace Engine
{
	namespace Resource 
	{ 
		enum class ResourceType
		{
			Texture,
		};
	}

	typedef uint64_t HashIndex;

	class ResourceAllocator
	{
	public: 
		static ResourceAllocator& GetInstance()
		{
			static ResourceAllocator instance;

			return instance;
		}

		//Safer for Singleton.
		ResourceAllocator(ResourceAllocator const&) = delete;
		void operator=(ResourceAllocator const&) = delete;

	public:
		ResourceAllocator();
		~ResourceAllocator();

		template <typename T>
		T& GetResource(HashIndex a_ID);
		template <typename T>
		T& GetResource(const char* a_FilePath, Resource::ResourceType a_Type);

		HashIndex LoadResource(const char* a_FilePath, Resource::ResourceType a_Type);

		void UnloadResource(const char* a_FilePath);
		void UnloadResource(HashIndex a_ID);

		void SetRenderer(Renderer* a_Renderer) { p_Renderer = a_Renderer; }

	private:
		HashIndex CreateTexture(const char* a_FilePath);


		HashIndex GetHashFromPath(const char* a_FilePath);

	private:
		std::unordered_map<HashIndex, Resource::Resource*> m_Resources;
		Renderer* p_Renderer;
	};


	template<typename T>
	inline T& ResourceAllocator::GetResource(HashIndex a_ID)
	{
		Resource::Resource& t_Resource = m_Resources.at(a_ID);

		return *dynamic_cast<T*>(&t_Resource);
	}

	template<typename T>
	inline T& ResourceAllocator::GetResource(const char* a_FilePath, Resource::ResourceType a_Type)
	{
		HashIndex t_HashIndex = GetHashFromPath(a_FilePath);

		auto t_Iterator = m_Resources.find(t_HashIndex);

		//If you cannot find it create and return the resource.
		if (t_Iterator != m_Resources.end())
		{
			return *dynamic_cast<T*>(m_Resources.at(t_HashIndex));
		}

		return *dynamic_cast<T*>(m_Resources.at(LoadResource(a_FilePath, a_Type)));
	}
};