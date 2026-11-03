#pragma once
#include "Component/Component.h"

namespace Dlight
{
	class Entity
	{
	public:
		explicit Entity(const std::string& entityName);
		virtual ~Entity() = default;

	public:
		void Initialize();
		void Update(float deltaTime);
		void Render();

		template<typename T, typename... Args>
		T* AddComponent(Args&&... args)
		{
			static_assert(std::is_base_of_v<Component, T>);

			uint64 typeID = ComponentTypeIDSystem::GetTypeID<T>();
			auto iter = componentMap.find(typeID);
			if (iter != componentMap.end())
			{
				return static_cast<T*>(iter->second);
			}

			auto component = std::make_unique<T>(std::forward<Args>(args)...);
			T* componentPtr = component.get();
			componentPtr->SetOwner(this);
	
			components.push_back(std::move(component));
			componentMap[typeID] = componentPtr;
			
			return componentPtr;
		}

		template<typename T>
		T* GetComponent()
		{
			uint64 typeID = Component::GetTypeID<T>();

			auto iter = componentMap.find(typeID);
			if (iter != componentMap.end())
			{
				return static_cast<T*>(iter->second);
			}

			return nullptr;
		}

		template<typename T>
		bool RemoveComponent()
		{
			uint64 typeID = Component::GetTypeID<T>();
			auto iter = componentMap.find(typeID);
			if (iter != componentMap.end())
			{
				Component* componentPtr = iter->second;
				componentMap.erase(iter);

				for (auto compIter = components.begin(); compIter != components.end(); ++compIter)
				{
					if (compIter->get() == componentPtr)
					{
						components.erase(compIter);
						return true;
					}
				}
			}

			return false;
		}
	public:
		const std::string& GetName() const { return name; };
		bool IsActive() { return isActive; };


	private:
		std::string name;
		bool isActive = { true };
		std::vector<std::unique_ptr<Component>> components;
		std::unordered_map<uint64, Component*> componentMap;
	};
}
