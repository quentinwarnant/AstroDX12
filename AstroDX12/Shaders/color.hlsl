struct VSInput
{

};

struct PSInput
{

};

PSInput VS(VSInput i)
{
	PSInput o;
	return o;
}


float4 PS(PSInput i) : SV_Target
{
	return float4(1, 0, 0, 1);
}
