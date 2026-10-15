#pragma once

namespace Dlight
{
    class MeshComponent
    {
    public:
        MeshComponent();
        ~MeshComponent();

    private:
        Mesh* mesh = { nullptr };
        Material* meterail = { nullptr };
    };
}
