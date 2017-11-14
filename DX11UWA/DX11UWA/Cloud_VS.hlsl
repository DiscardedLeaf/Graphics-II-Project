cbuffer perObject : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	matrix inverseTransposeWorld;
}

struct VS_Input
{
	float4 pos : POSITION;
};

struct VS_Output
{
	float4 pos : SV_POSITION;
};

VS_Output main( VS_Input input )
{
	VS_Output output;

	//get into view space for geo-shader to work
	float4 pos = input.pos;
	pos = mul(pos, world);
	pos = mul(pos, view);

	output.pos = pos;

	return output;
}