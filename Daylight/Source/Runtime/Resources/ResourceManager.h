#pragma once
#include "Resource.h"

namespace Dlight
{
	class ResourceManager
	{
	public:
		ResourceManager() = default;
		~ResourceManager() {};

	public:
		template<typename T, typename... Args>
		ResourceHandle<T> LoadResource(const std::string& id, Args&& ...args)
		{
			static_assert(std::is_base_of_v<Resource, T>, "T Must derive from Resource");

			auto& typeResources = resources[std::type_index(typeid(T))];
			auto iter = typeResources.find(id);

			if (iter != typeResources.end())
			{
				return ResourceHandle<T>(std::static_pointer_cast<T>(iter->second));
			}

			std::shared_ptr<T> resource(new T(id, std::forward<Args>(args)...)
				, [](T* r) { r->UnLoad(); delete r; });

			if (!resource->Load())
			{
				throw std::runtime_error(
					"Failed to load resource: " + id);
			}

			typeResources[id] = resource;

			return ResourceHandle<T>(resource);
		}
		template<typename T>
		bool UnloadResource(const std::string& id)
		{
			static_assert(std::is_base_of_v<Resource, T>, "T Must drive from Resource");
			
			auto& typeResources = resources[std::type_index(typeid(T))];
			auto iter = typeResources.find(id);
			if (iter == typeResources.end())
			{
				return false;
			}

			typeResources.erase(iter);
			return true;
		}
		template<typename T>
		ResourceHandle<T> GetResource(const std::string& id)
		{
			static_assert(std::is_base_of_v<Resource, T>, "T Must drive from Resource");
			auto& typeResources = resources[std::type_index(typeid(T))];
			auto iter = typeResources.find(id);
			
			if (iter != typeResources.end())
			{
				return ResourceHandle<T>(std::static_pointer_cast<T>(iter->second));
			}

			return ResourceHandle<T>();
		}

		template<typename T>
		bool HasResource(const std::string& id)
		{
			static_assert(std::is_base_of_v<Resource, T>, "T Must drive from Resource");
			auto& typeResources = resources[std::type_index(typeid(T))];
			auto iter = typeResources.find(id);

			if (iter != typeResources.end())
			{
				return true;
			}

			return false;
		}

		void UnloadAllResource()
		{
			resources.clear();
		}

	private:
		std::unordered_map<std::type_index,
			std::unordered_map<std::string, std::shared_ptr<Resource>>> resources;
	};
}
