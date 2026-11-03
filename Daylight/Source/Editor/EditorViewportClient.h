#pragma once

#include "CoreTypes.h"
#include "Rendering/ViewData.h"

union SDL_Event;

namespace Dlight
{
    class EditorViewportClient
    {
    private:
        struct FCameraState
        {
            bool movingForward = { false };
            bool movingBackward = { false };
            bool movingLeft = { false };
            bool movingRight = { false };
            bool movingUp = { false };
            bool movingDown = { false };
        };

        class EditorCamera
        {
        public:
            EditorCamera() = default;
            ~EditorCamera() = default;

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
			void Orbit(float deltaX, float deltaY);
			void Zoom(float wheelDelta);

        private:
            friend class EditorViewportClient;

            float nearPlane = { 0.1f };
            float farPlane = { 1000.f };
            float aspectRatio = { 16.f / 9.f };
            float FOV = {};

			Vector3    position = { 0.0f, 0.0f, -3.0f };
            Quaternion rotation = { Quaternion::Identity };

			Vector3 orbitTarget = { Vector3::Zero };
			float orbitDistance = { 3.0f };
			float orbitYaw = { 0.0f };
			float orbitPitch = { 0.0f };
			float orbitSensitivity = { 0.005f };

            mutable Matrix projectionMatrix = { Matrix::Identity };
            mutable bool projectionDirty = { true };

            float speed = { 5.f };
        };

    public:
        EditorViewportClient();
        ~EditorViewportClient();

        void HandleEvent(const SDL_Event& event);
        void Tick(float DeltaTime);
        FViewData BuildViewData(uint32 width, uint32 height);

    private:
        EditorCamera camera = {};
        FCameraState cameraState = {};
    };
}
