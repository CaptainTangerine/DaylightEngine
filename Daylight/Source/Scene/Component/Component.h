#pragma once

namespace Dlight
{
    class Component
    {
    public:
        Component() = default;
        virtual ~Component() = default;

    public:
        virtual void Initialize() {};
        virtual void Update(float deltaTime) {};
        virtual void Render() {};

        void SetOnwer(Entity* entity) { onwer = entity; };
        Entity* GetOnwer() const { return onwer; };

    protected:
        friend class Entity;
        
        Entity* onwer = { nullptr };
    };

}