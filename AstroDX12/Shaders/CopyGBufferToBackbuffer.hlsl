
struct VSInput
{
    uint VertexID : SV_VertexID;
    float2 UV : TEXCOORD0;
};

struct PSInput
{
	float4 PosH  : SV_POSITION;
    float2 UV : TEXCOORD0;
};

cbuffer BindlessRenderResources : register(b0)
{
    int GBufferRTResourceIndex;
    int GBufferWidth;
    int GBufferHeight;
};

PSInput VS(VSInput i)
{
    const float2 Positions[4] =
    {
        // Full screen quad
        //float2(-1, -1),
        //float2(-1, 1),
        //float2(1, -1),
        //float2(1, 1),
        // Quarter screen size
        float2(0, 0),
        float2(0, 1),
        float2(1, 0),
        float2(1, 1),
    };
    
    const float2 UVs[4] =
    {
        float2(0, 0),
        float2(0, 1),
        float2(1, 0),
        float2(1, 1),
    };
    
    
    PSInput o;

    o.PosH = float4(Positions[i.VertexID], 0, 1.0f);
    o.UV = UVs[i.VertexID];

    return o;
}

float4 PS(PSInput i) : SV_Target
{
    Texture2D<float4> GBuffer = ResourceDescriptorHeap[GBufferRTResourceIndex];
    const float2 ScreenSize = float2(GBufferWidth, GBufferHeight);
    
    const float EdgeWidth = 0.001f; // Width of the edge detection area
    bool isEdge = i.UV.x <= EdgeWidth || i.UV.x > 1 - EdgeWidth || i.UV.y < EdgeWidth || i.UV.y > 1 - EdgeWidth;
    const float4 color = isEdge ? float4(1, 0, 1, 1) : GBuffer[i.UV * ScreenSize];
    
    return color;
}
