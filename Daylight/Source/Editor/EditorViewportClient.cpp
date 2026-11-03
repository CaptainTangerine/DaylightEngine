#include "pch.h"
#include "EditorViewportClient.h"
#include <SDL3/SDL.h>

namespace Dlight
{
    EditorViewportClient::EditorViewportClient()
    {
        camera.SetPerspective(DirectX::XMConvertToRadians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
    }

    EditorViewportClient::~EditorViewportClient()
    {
    }

    void EditorViewportClient::Tick(float DeltaTime)
    {
        if (cameraState.movingForward)
        {
            camera.position.z += DeltaTime * camera.speed;
        }
        if (cameraState.movingBackward)
        {
            camera.position.z -= DeltaTime * camera.speed;
        }
        if (cameraState.movingLeft)
        {
            camera.position.x -= DeltaTime * camera.speed;
        }
        if (cameraState.movingRight)
        {
            camera.position.x += DeltaTime * camera.speed;
        }
        if (cameraState.movingUp)
        {
            camera.position.y += DeltaTime * camera.speed;
        }
        if (cameraState.movingDown)
        {
            camera.position.y -= DeltaTime * camera.speed;
        }

    }

    FViewData EditorViewportClient::BuildViewData(uint32 width, uint32 height)
    {
        if (width > 0 && height > 0)
        {
            const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
            if (aspectRatio != camera.aspectRatio)
            {
                camera.SetPerspective(camera.FOV, aspectRatio, camera.nearPlane, camera.farPlane);
            }
        }

        FViewData viewData = {};
        viewData.viewMatrix = camera.GetViewMatrix();
        viewData.projectionMatrix = camera.GetProjectionMatrix();
        return viewData;
    }

    void EditorViewportClient::HandleEvent(const SDL_Event& event)
    {
        if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
        {
            cameraState = {};
            return;
        }

        if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP)
        {
            return;
        }

        const bool pressed = event.type == SDL_EVENT_KEY_DOWN;
        switch (event.key.scancode)
        {
        case SDL_SCANCODE_W:
            cameraState.movingForward = pressed;
            break;
        case SDL_SCANCODE_S:
            cameraState.movingBackward = pressed;
            break;
        case SDL_SCANCODE_A:
            cameraState.movingLeft = pressed;
            break;
        case SDL_SCANCODE_D:
            cameraState.movingRight = pressed;
            break;
        case SDL_SCANCODE_Q:
            cameraState.movingUp = pressed;
            break;
        case SDL_SCANCODE_E:
            cameraState.movingDown = pressed;
            break;
        default:
            break;
        }
    }

    Matrix EditorViewportClient::EditorCamera::GetViewMatrix() const
    {
        Vector3 up = Vector3::Transform(Vector3::UnitY, rotation);
        Vector3 forward = Vector3::Transform(Vector3::UnitZ, rotation);
        
        return DirectX::XMMatrixLookAtLH(position, position + forward, up);
    }
    Matrix EditorViewportClient::EditorCamera::GetProjectionMatrix() const
    {
        if (projectionDirty)
        {
            projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(FOV, aspectRatio, nearPlane, farPlane);
            projectionDirty = false;
        }

        return projectionMatrix;
    }
}
