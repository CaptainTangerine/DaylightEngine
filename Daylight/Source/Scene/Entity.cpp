#include "pch.h"
#include "Entity.h"
#include "Component/Component.h"

namespace Dlight
{

	Entity::Entity(const std::string& entityName)
		: name(entityName)
	{

	}

	void Entity::Initialize()
	{
		for (auto& component : components)
		{
			component->Initialize();
		}
	}

	void Entity::Update(float deltaTime)
	{
		if (!isActive) return;

		for (auto& component : components)
		{
			component->Update(deltaTime);
		}
	}

	void Entity::Render()
	{
		if (!isActive) return;

		for (auto& component : components)
		{
			component->Render();
		}
	}

}