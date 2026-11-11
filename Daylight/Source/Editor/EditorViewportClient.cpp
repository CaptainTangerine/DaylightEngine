#include "pch.h"
#include "EditorViewportClient.h"
#include <SDL3/SDL.h>

#include <cmath>

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
		const Vector3 forward = Vector3::Transform(Vector3::UnitZ, camera.rotation);
		const Vector3 right = Vector3::Transform(Vector3::UnitX, camera.rotation);
		const float moveDistance = DeltaTime * camera.speed;

		Vector3 translation = Vector3::Zero;
        if (cameraState.movingForward)
        {
			translation += forward * moveDistance;
        }
        if (cameraState.movingBackward)
        {
			translation -= forward * moveDistance;
        }
        if (cameraState.movingLeft)
        {
			translation -= right * moveDistance;
        }
        if (cameraState.movingRight)
        {
			translation += right * moveDistance;
        }
        if (cameraState.movingUp)
        {
			translation += Vector3::UnitY * moveDistance;
        }
        if (cameraState.movingDown)
        {
			translation -= Vector3::UnitY * moveDistance;
        }

		camera.position += translation;
		camera.orbitTarget += translation;
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
        viewData.cameraPosition = camera.position;
        return viewData;
    }

    void EditorViewportClient::HandleEvent(const SDL_Event& event)
    {
        if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
        {
            cameraState = {};
            return;
        }

		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			if ((event.motion.state & SDL_BUTTON_LMASK) != 0 &&
				(SDL_GetModState() & SDL_KMOD_ALT) != 0)
			{
				camera.Orbit(event.motion.xrel, event.motion.yrel);
			}
			return;
		}

		if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			float wheelDelta = event.wheel.y;
			if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
			{
				wheelDelta = -wheelDelta;
			}

			camera.Zoom(wheelDelta);
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

	void EditorViewportClient::EditorCamera::Orbit(float deltaX, float deltaY)
	{
		orbitYaw += deltaX * orbitSensitivity;
		orbitPitch += deltaY * orbitSensitivity;

		constexpr float pitchLimit = DirectX::XMConvertToRadians(89.0f);
        orbitPitch = std::clamp(orbitPitch, -pitchLimit, pitchLimit);

		rotation = Quaternion::CreateFromYawPitchRoll(orbitYaw, orbitPitch, 0.0f);
		const Vector3 forward = Vector3::Transform(Vector3::UnitZ, rotation);
		position = orbitTarget - forward * orbitDistance;
	}

	void EditorViewportClient::EditorCamera::Zoom(float wheelDelta)
	{
		constexpr float zoomFactorPerStep = 0.85f;
		constexpr float minOrbitDistance = 0.1f;
		constexpr float maxOrbitDistance = 1000.0f;

		orbitDistance *= std::pow(zoomFactorPerStep, wheelDelta);
		orbitDistance = std::clamp(orbitDistance, minOrbitDistance, maxOrbitDistance);

		const Vector3 forward = Vector3::Transform(Vector3::UnitZ, rotation);
		position = orbitTarget - forward * orbitDistance;
	}
}
