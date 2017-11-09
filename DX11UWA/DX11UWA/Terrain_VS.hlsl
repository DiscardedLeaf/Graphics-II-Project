
Texture2D Texture : register(t0); //the terrain map
sampler Sampler : register(s0); //to read the terrain map


cbuffer PerObject : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	matrix inverseTransposeWorld;
};

cbuffer ShadowMap : register(b1)
{
	float width;
	float height;
}

struct VS_Input
{
	float3 pos : POSITION;
	float3 uv : TEXCOORD;
	float3 norm : NORMAL;
};
struct VS_Output
{
	float4 posWS : TEXCOORD1;
	float3 normWS : TEXCOORD2;
	float2 uv : TEXCOORD0;
	float4 pos : SV_POSITION;
};


VS_Output main(VS_Input input)
{
	VS_Output output;
	return output;
}