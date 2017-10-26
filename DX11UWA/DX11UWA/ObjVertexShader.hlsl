cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 uv : UV;
	float3 norm : NORMAL;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 norm : NORMAL
};

PixelShaderInput main( VertexShaderInput input )
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);
	float4 norm = float4(input.norm, 0.0f);

	pos = mul(pos, model);
	pos = mul(pos, view);
	pos = mul(pos, projection);
	norm = mul(norm, model);
	norm = mul(norm, view);
	norm = mul(norm, projection);

	output.pos = pos;
	output.norm = (float3)norm;
	output.uv = input.uv;

	return output;
}