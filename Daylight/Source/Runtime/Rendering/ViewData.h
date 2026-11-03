#pragma once

#include "Math/MathTypes.h"

namespace Dlight
{
    // 한 프레임의 카메라 데이터. Editor 타입이나 카메라 객체를 참조하지 않는다.
    struct FViewData
    {
        Matrix viewMatrix = { Matrix::Identity };
        Matrix projectionMatrix = { Matrix::Identity };
    };
}
