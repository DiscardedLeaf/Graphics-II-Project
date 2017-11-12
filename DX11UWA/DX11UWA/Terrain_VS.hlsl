cbuffer perObject : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	matrix inverseTransposeWorld;
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

	float4 position = float4(input.pos, 1.0f);
	float4 normal = float4(input.norm, 0.0f);

	position = mul(position, world);
	position = mul(position, view);
	position = mul(position, projection);

	output.pos = position;
	output.posWS = mul(float4(input.pos, 1.0f), world);
	output.normWS = mul(normal, world).xyz;
	output.uv = input.uv.xy;

	return output;
}