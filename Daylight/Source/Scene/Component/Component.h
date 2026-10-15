#pragma once

namespace Dlight
{
    enum class EComponentState
    {
        UnInitialized,
        Initializing,
        Active,
        Destroying,
        Destroyed,
    };

    class Entity;
    class ComponentTypeIDSystem
    {
    private:
        static std::atomic_size_t nextTypeID;

    public:
        template<typename T>
        static uint64 GetTypeID()
        {
            // 초기화는 템플릿 인스턴스화 시점에만 한번
            static uint64 typeID = nextTypeID++;
            return typeID;
        }
    };


    class Component
    {
    public:
        Component() = default;
        virtual ~Component() = default;

    public:
        virtual void Initialize() 
        {
            if (eState == EComponentState::UnInitialized)
            {
                eState = EComponentState::Initializing;
                OnInitialize();
                eState = EComponentState::Active;
            }

        };
        virtual void Destroy() 
        {
            if (eState == EComponentState::Active)
            {
                eState = EComponentState::Destroying;
                OnDestroy();
                eState = EComponentState::Destroyed;
            }
        };
    public:
        template<typename T>
        static uint64 GetTypeID() {
            return ComponentTypeIDSystem::GetTypeID<T>();
        }

        void SetOwner(Entity* entity) { onwer = entity; };
        Entity* GetOnwer() const { return onwer; };

    protected:
        virtual void OnInitialize() {};
        virtual void Update(float deltaTime) {};
        virtual void Render() {};
        virtual void OnDestroy() {};

    protected:
        friend class Entity;
        
        Entity* onwer = { nullptr };
        EComponentState eState = { EComponentState::UnInitialized };

    };

}
