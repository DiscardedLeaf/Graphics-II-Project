cbuffer PerObject : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	matrix inverseTransposeWorld;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 uv : TEXCOORD;
	float3 norm : NORMAL;
};

struct VertexShaderOutput
{
	float4 posWS : TEXCOORD1;
	float3 normWS : TEXCOORD2;
	float2 uv : TEXCOORD0;
	float4 pos : SV_Position;
};

VertexShaderOutput main( VertexShaderInput input )
{
	VertexShaderOutput output;
	
	float4 pos = float4(input.pos, 1.0f);
	float4 norm = float4(input.norm, 0.0f);

	pos = mul(pos, world);
	pos = mul(pos, view);
	pos = mul(pos, projection);
	norm = mul(norm, world);
	output.pos = pos;
	output.posWS = mul( float4(input.pos, 1.0f), world);
	output.normWS = (float3)norm;
	output.uv = input.uv.xy;

	return output;
}