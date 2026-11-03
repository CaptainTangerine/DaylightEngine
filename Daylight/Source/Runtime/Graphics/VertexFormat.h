#pragma once

namespace Dlight
{
	namespace VertexFormat
	{
		struct FColoredVertex
		{
			Vector3 position;
			Vector3 color;
		};

		struct FTexturedVertex
		{
			Vector2 position;
			Vector2 texcoord;
		};
	}
}
