#pragma once

#include "Graphics/VertexFormat.h"
#include "Math/MathConstants.h"

#include <cmath>
#include <vector>

namespace Dlight::SphereMeshData
{
	struct FSphereMeshData
	{
		std::vector<VertexFormat::FPBRVertex> vertices;
		std::vector<uint32> indices;
	};

	inline void CalculateTangents(FSphereMeshData& meshData)
	{
		std::vector<Vector3> tangentSums(meshData.vertices.size(), Vector3(0.0f, 0.0f, 0.0f));
		std::vector<Vector3> bitangentSums(meshData.vertices.size(), Vector3(0.0f, 0.0f, 0.0f));

		for (size_t index = 0; index + 2 < meshData.indices.size(); index += 3)
		{
			const uint32 index0 = meshData.indices[index];
			const uint32 index1 = meshData.indices[index + 1];
			const uint32 index2 = meshData.indices[index + 2];

			const VertexFormat::FPBRVertex& vertex0 = meshData.vertices[index0];
			const VertexFormat::FPBRVertex& vertex1 = meshData.vertices[index1];
			const VertexFormat::FPBRVertex& vertex2 = meshData.vertices[index2];

			const Vector3 edge1 = vertex1.position - vertex0.position;
			const Vector3 edge2 = vertex2.position - vertex0.position;
			const Vector2 deltaUv1 = vertex1.texcoord - vertex0.texcoord;
			const Vector2 deltaUv2 = vertex2.texcoord - vertex0.texcoord;
			const float determinant = deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x;

			if (std::abs(determinant) <= Math::Epsilon)
			{
				continue;
			}

			const float inverseDeterminant = 1.0f / determinant;
			const Vector3 tangent = (edge1 * deltaUv2.y - edge2 * deltaUv1.y) * inverseDeterminant;
			const Vector3 bitangent = (edge2 * deltaUv1.x - edge1 * deltaUv2.x) * inverseDeterminant;

			tangentSums[index0] += tangent;
			tangentSums[index1] += tangent;
			tangentSums[index2] += tangent;
			bitangentSums[index0] += bitangent;
			bitangentSums[index1] += bitangent;
			bitangentSums[index2] += bitangent;
		}

		for (size_t vertexIndex = 0; vertexIndex < meshData.vertices.size(); ++vertexIndex)
		{
			VertexFormat::FPBRVertex& vertex = meshData.vertices[vertexIndex];
			const Vector3& normal = vertex.normal;
			Vector3 tangent = tangentSums[vertexIndex] - normal * normal.Dot(tangentSums[vertexIndex]);

			if (tangent.LengthSquared() <= Math::Epsilon)
			{
				const float theta = vertex.texcoord.x * 2.0f * Math::Pi;
				tangent = Vector3(-std::sin(theta), 0.0f, std::cos(theta));
			}

			tangent.Normalize();
			const float handedness = normal.Cross(tangent).Dot(bitangentSums[vertexIndex]) < 0.0f ? -1.0f : 1.0f;
			vertex.tangent = Vector4(tangent.x, tangent.y, tangent.z, handedness);
		}
	}

	inline FSphereMeshData Create(float radius = 0.75f, uint32 latitudeSegments = 32, uint32 longitudeSegments = 64)
	{
		FSphereMeshData meshData;
		meshData.vertices.reserve(static_cast<size_t>(latitudeSegments + 1) * (longitudeSegments + 1));
		meshData.indices.reserve(static_cast<size_t>(latitudeSegments) * longitudeSegments * 6);

		const Vector3 Red = { 1.0f, 0.0f, 0.0f };

		for (uint32 latitude = 0; latitude <= latitudeSegments; ++latitude)
		{
			const float v = static_cast<float>(latitude) / static_cast<float>(latitudeSegments);
			const float phi = v * Math::Pi;
			const float sinPhi = std::sin(phi);
			const float cosPhi = std::cos(phi);

			for (uint32 longitude = 0; longitude <= longitudeSegments; ++longitude)
			{
				const float u = static_cast<float>(longitude) / static_cast<float>(longitudeSegments);
				const float theta = u * 2.0f * Math::Pi;

				const Vector3 normal = { sinPhi * std::cos(theta), cosPhi, sinPhi * std::sin(theta) };
				const Vector3 position = { normal.x * radius, normal.y * radius, normal.z * radius };
				const Vector2 texcoord = { u, v };
				meshData.vertices.push_back({ position, Red, normal, texcoord, Vector4(0.0f, 0.0f, 0.0f, 1.0f) });
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

		CalculateTangents(meshData);

		return meshData;
	}
}
