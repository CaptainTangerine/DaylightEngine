#pragma once

namespace Dlight
{
	class Component;
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
			static_assert(std::is_base_of<Component, T>);
			auto component = std::make_unique<T>(std::forward<Args>(args)...);
			T* componentPtr = component.get();
			componentPtr->SetOwner(this);

			components.push_back(std::move(componentPtr));
			
			return componentPtr;
		}

		template<typename T>
		T* GetComponent()
		{
			for (auto& component : components)
			{
				if (T* result = dynamic_cast<T*>(component))
				{
					return result;
				}
			}
		}

		template<typename T>
		bool RemoveComponent()
		{
			for (auto iter = components.begin(); iter != components.end(); ++iter)
			{
				if (dynamic_cast<T*>(iter->get()))
				{
					components.erase(iter);
					return true;
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
	};
}
