#include "pch.h"
#include "CameraComponent.h"

#include "Scene/Entity.h"
#include "TransformComponent.h"


namespace Dlight
{
    CameraComponent::CameraComponent()
    {
    }

    CameraComponent::~CameraComponent()
    {
    }
    Matrix CameraComponent::GetViewMatrix() const
    {
        auto transform = GetOnwer()->GetComponent<TransformComponent>();
        if (transform)
        {
            Vector3 scale = transform->GetScale();
            Quaternion rotation = transform->GetRotation();
            Vector3 position = transform->GetPosition();

            Vector3 up = Vector3::Transform(Vector3::UnitY, rotation);
            Vector3 forward = Vector3::Transform(Vector3::UnitZ, rotation);

            return DirectX::XMMatrixLookAtLH(transform->GetPosition(), position + forward, up);
        }

        return Matrix::Identity;
    }
    Matrix CameraComponent::GetProjectionMatrix() const
    {
        if (projectionDirty)
        {
            DirectX::XMMatrixPerspectiveFovLH(FOV, aspectRatio, nearPlane, farPlane);
            projectionDirty = false;
        }
        return ProjectionMatrix;
    }
}
