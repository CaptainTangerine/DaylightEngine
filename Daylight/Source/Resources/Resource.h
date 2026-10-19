#pragma once

namespace Dlight
{
    class ResourceManager;

    template<typename T>
    class ResourceHandle
    {
    public:
        ResourceHandle() = default;
        explicit ResourceHandle(std::shared_ptr<T> _resource) :
            resource(std::move(_resource)) {}

        T* Get() const
        {
            return resource.get();
        }

        bool IsValid() const { return resource != nullptr; }
        
        const std::string& GetID() const { return resource->GetID(); };
        T* operator->() const { return Get(); }
        T& operator*() const { return *Get(); }

    private:
        std::shared_ptr<T> resource;
    };


    class Resource
    {
    public:
        Resource(const std::string& id) : resourceID(id) {};
        virtual ~Resource() = default;
        
        const std::string& GetID() { return resourceID; }

        bool IsLoaded() const { return loaded; };
        bool Load() 
        {
            loaded = DoLoad(); 
            return loaded; 
        }

        void UnLoad()
        {
            DoUnload();
            loaded = false;
        }

    protected:
        virtual bool DoLoad() = 0;
        virtual void DoUnload() = 0;
    
    private:
        std::string resourceID;
        bool loaded = { false };
    };
}
