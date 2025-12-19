#pragma once

#include <vector>
#include <map>
#include <cmath>
#include <DirectXMath.h>


using float3 = DirectX::XMFLOAT3;

struct Geometry 
{
    std::vector<float3> vertices;
    std::vector<uint32_t> indices;
};

namespace GeometryHelper
{
    // Helper to get or create a midpoint vertex
    static uint32_t GetMidpoint(uint32_t p1, uint32_t p2, std::vector<float3>& vertices, std::map<uint64_t, uint32_t>& cache) {
        // Check if we already created this midpoint
        uint64_t key = (uint64_t)std::min(p1, p2) << 32 | std::max(p1, p2);
        if (cache.count(key)) return cache[key];

        float3 v1 = vertices[p1];
        float3 v2 = vertices[p2];

        // Calculate midpoint
        float3 mid = { (v1.x + v2.x) / 2.0f, (v1.y + v2.y) / 2.0f, (v1.z + v2.z) / 2.0f };

        // Project to unit sphere (assuming radius 1.0)
        float length = std::sqrt(mid.x * mid.x + mid.y * mid.y + mid.z * mid.z);
        mid.x /= length; mid.y /= length; mid.z /= length;

        uint32_t id = (uint32_t)vertices.size();
        vertices.push_back(mid);
        cache[key] = id;
        return id;
    }

    static Geometry GenerateDelaunaySphere(int subdivisions) {
        Geometry mesh;
        const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

        // 1. Create 12 vertices of an Icosahedron
        mesh.vertices = {
            {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
            { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
            { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
        };

        // Normalize initial vertices to make it a unit sphere
        for (auto& v : mesh.vertices) {
            float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            v.x /= len; v.y /= len; v.z /= len;
        }

        // 2. Initial 20 faces
        mesh.indices = {
            0, 11, 5,  0, 5, 1,  0, 1, 7,  0, 7, 10,  0, 10, 11,
            1, 5, 9,  5, 11, 4,  11, 10, 2,  10, 7, 6,  7, 1, 8,
            3, 9, 4,  3, 4, 2,  3, 2, 6,  3, 6, 8,  3, 8, 9,
            4, 9, 5,  2, 4, 11,  6, 2, 10,  8, 6, 7,  9, 8, 1
        };

        // 3. Subdivide
        std::map<uint64_t, uint32_t> midpointCache;
        for (int i = 0; i < subdivisions; ++i) {
            std::vector<uint32_t> nextIndices;
            for (size_t j = 0; j < mesh.indices.size(); j += 3) {
                uint32_t a = mesh.indices[j];
                uint32_t b = mesh.indices[j + 1];
                uint32_t c = mesh.indices[j + 2];

                uint32_t ab = GetMidpoint(a, b, mesh.vertices, midpointCache);
                uint32_t bc = GetMidpoint(b, c, mesh.vertices, midpointCache);
                uint32_t ca = GetMidpoint(c, a, mesh.vertices, midpointCache);

                // Replace 1 triangle with 4
                uint32_t subTriangles[] = {
                    a, ab, ca,
                    b, bc, ab,
                    c, ca, bc,
                    ab, bc, ca
                };
                nextIndices.insert(nextIndices.end(), std::begin(subTriangles), std::end(subTriangles));
            }
            mesh.indices = nextIndices;
        }

        return mesh;
    }
}
