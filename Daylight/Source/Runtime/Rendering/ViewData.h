#pragma once

#include "Math/MathTypes.h"

namespace Dlight
{
	struct alignas(16) FPerViewUniform
	{
		Matrix viewMatrix;
		Matrix projectionMatrix;
		Matrix inverseViewMatrix;
		Matrix inverseProjectionMatrix;
		Vector3 cameraPosition;
	};

    // 한 프레임의 카메라 데이터. Editor 타입이나 카메라 객체를 참조하지 않는다.
    struct FViewData
    {
        Matrix viewMatrix = { Matrix::Identity };
        Matrix projectionMatrix = { Matrix::Identity };
        Vector3 cameraPosition = { Vector3::Zero };
    };
}
