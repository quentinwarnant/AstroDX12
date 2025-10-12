#pragma once

struct ParticleData
{
    float3 Pos;
    float3 PrevPos;
    float3x3 Rot;
};

struct ChainElementData
{
    ParticleData Particle;
    int ParentIndex;
    float RestLength;
    bool Pinned;
};