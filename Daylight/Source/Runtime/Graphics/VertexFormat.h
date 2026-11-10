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

		struct FColoredNormalVertex
		{
			Vector3 position;
			Vector3 color;
			Vector3 normal;
		};

		struct FPBRVertex
		{
			Vector3 position;
			Vector3 color;
			Vector3 normal;
			Vector2 texcoord;
			Vector4 tangent;
		};

		struct FTexturedVertex
		{
			Vector2 position;
			Vector2 texcoord;
		};
	}
}
