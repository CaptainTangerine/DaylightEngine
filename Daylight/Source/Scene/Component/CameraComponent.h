#pragma once
#include "Component.h"

namespace Dlight
{
    class TransformComponent;
    class Entity;

    class CameraComponent : Component
    {
    public:
        CameraComponent();
        ~CameraComponent();

    public:
        void SetPerspective(float fov, float aspect, float n, float f) 
        {
            FOV = fov;
            aspectRatio = aspect;
            nearPlane = n;
            farPlane = f;
            projectionDirty = true;
        }

        Matrix GetViewMatrix() const
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

        Matrix GetProjectionMatrix() const
        {
            if (projectionDirty)
            {
                DirectX::XMMatrixPerspectiveFovLH(FOV, aspectRatio, nearPlane, farPlane);
                projectionDirty = false;
            }
            return ProjectionMatrix;
        }

    private:
        float nearPlane = { 0.1f };
        float farPlane = { 1000.f };
        float aspectRatio = { 16.f / 9.f };
        float FOV = {};

        Matrix ViewMatrix = { Matrix::Identity };

        mutable Matrix ProjectionMatrix = { Matrix::Identity };
        mutable bool projectionDirty = { ture };
    };
}
