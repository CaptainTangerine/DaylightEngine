#pragma once
#include "Math/MathTypes.h"

namespace Dlight
{
	namespace VertexFormat
	{
		struct FColoredVertex
		{
			Vector2 position;
			Vector3 color;
		};

		struct FTexturedVertex
		{
			Vector2 position;
			Vector2 texcoord;
		};
	}
}
