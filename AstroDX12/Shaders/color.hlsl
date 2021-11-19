cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
};

struct VSInput
{
	float3 PosL  : POSITION;
	float4 Color : COLOR;
};

struct PSInput
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
};

PSInput VS(VSInput i)
{
	PSInput o;

	// Transform to homogeneous clip space.
	o.PosH = mul(float4(i.PosL, 1.0f), gWorldViewProj);
	o.Color = i.Color;

	return o;
}


float4 PS(PSInput i) : SV_Target
{
	return i.Color;
}
