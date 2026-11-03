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
        // MeshResource Handle
        // MaterialResource Handle

    };
}
