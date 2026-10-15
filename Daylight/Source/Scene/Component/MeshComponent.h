#pragma once
#include "Component.h"

namespace Dlight
{
    class MeshComponent : public Component
    {
    public:
        MeshComponent();
        ~MeshComponent();

        void Render() override;

    private:
        // Vertex Data 
        // Index Data
        // Material Path

    };
}
