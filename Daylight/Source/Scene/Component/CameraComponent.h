#pragma once
#include "Component.h"

namespace Dlight
{
    class TransformComponent;
    class Entity;

    class CameraComponent : public Component
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

        Matrix GetViewMatrix() const;
        Matrix GetProjectionMatrix() const;

    private:
        float nearPlane = { 0.1f };
        float farPlane = { 1000.f };
        float aspectRatio = { 16.f / 9.f };
        float FOV = {};

        Matrix ViewMatrix = { Matrix::Identity };

        mutable Matrix ProjectionMatrix = { Matrix::Identity };
        mutable bool projectionDirty = { true };
    };
}
