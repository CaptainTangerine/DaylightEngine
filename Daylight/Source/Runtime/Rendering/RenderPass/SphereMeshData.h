#pragma once

#include "Graphics/VertexFormat.h"

#include <cmath>
#include <vector>

namespace Dlight::SphereMeshData
{
	struct FSphereMeshData
	{
		std::vector<VertexFormat::FPBRVertex> vertices;
		std::vector<uint32> indices;
	};

	inline FSphereMeshData Create(
		float radius = 0.75f,
		uint32 latitudeSegments = 32,
		uint32 longitudeSegments = 64)
	{
		FSphereMeshData meshData;
		meshData.vertices.reserve(
			static_cast<size_t>(latitudeSegments + 1) * (longitudeSegments + 1));
		meshData.indices.reserve(
			static_cast<size_t>(latitudeSegments) * longitudeSegments * 6);

		constexpr float Pi = 3.14159265358979323846f;
		const Vector3 Red = { 1.0f, 0.0f, 0.0f };

		for (uint32 latitude = 0; latitude <= latitudeSegments; ++latitude)
		{
			const float v = static_cast<float>(latitude) /
				static_cast<float>(latitudeSegments);
			const float phi = v * Pi;
			const float sinPhi = std::sin(phi);
			const float cosPhi = std::cos(phi);

			for (uint32 longitude = 0; longitude <= longitudeSegments; ++longitude)
			{
				const float u = static_cast<float>(longitude) /
					static_cast<float>(longitudeSegments);
				const float theta = u * 2.0f * Pi;

				const Vector3 normal =
				{
					sinPhi * std::cos(theta),
					cosPhi,
					sinPhi * std::sin(theta)
				};
				const Vector3 position =
				{
					normal.x * radius,
					normal.y * radius,
					normal.z * radius
				};
				const Vector2 texcoord = { u, v };
				const Vector4 tangent =
				{
					-std::sin(theta),
					0.0f,
					std::cos(theta),
					1.0f
				};

				meshData.vertices.push_back(
					{ position, Red, normal, texcoord, tangent });
			}
		}

		const uint32 rowSize = longitudeSegments + 1;
		for (uint32 latitude = 0; latitude < latitudeSegments; ++latitude)
		{
			for (uint32 longitude = 0; longitude < longitudeSegments; ++longitude)
			{
				const uint32 current = latitude * rowSize + longitude;
				const uint32 nextRow = current + rowSize;

				meshData.indices.push_back(current);
				meshData.indices.push_back(current + 1);
				meshData.indices.push_back(nextRow);

				meshData.indices.push_back(nextRow);
				meshData.indices.push_back(current + 1);
				meshData.indices.push_back(nextRow + 1);
			}
		}

		return meshData;
	}
}
