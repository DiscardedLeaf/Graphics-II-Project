//Texture2D Texture : register(t0);
//sampler Sampler : register(s0);

cbuffer perObject : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	matrix inverseTransposeWorld;
}

struct DS_OUTPUT
{
	float4 posWS : TEXCOORD1;
	float3 normWS : TEXCOORD2;
	float2 uv : TEXCOORD0;
	float4 pos  : SV_POSITION;
};

// Output control point
struct HS_CONTROL_POINT_OUTPUT
{
	float3 pos : POSITION;
	float3 uv : TEXCOORD;
	float3 norm : NORMAL;
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]			: SV_TessFactor; // e.g. would be [4] for a quad domain
	float InsideTessFactor[2]			: SV_InsideTessFactor; // e.g. would be Inside[2] for a quad domain
};

#define NUM_CONTROL_POINTS 4

[domain("quad")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float2 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT output;

	//find where along the u axis the vertex is
	float3 topMidpoint = lerp(patch[1].pos, patch[3].pos, domain.x);
	float3 botMidpoint = lerp(patch[0].pos, patch[2].pos, domain.x);

	//find where along the u the uv coordinate is
	float3 TopUVMidpoint = lerp(patch[1].uv, patch[3].uv, domain.x);
	float3 BotUVMidpoint = lerp(patch[0].uv, patch[2].uv, domain.x);

	//get base interpolated variables for the vertex
	float4 posWS = float4(lerp(topMidpoint, botMidpoint, domain.y), 1.0f);
	float4 pos = posWS;
	float3 normWS = float3(0.0f, 1.0f, 0.0f);
	float2 uv = lerp(TopUVMidpoint, BotUVMidpoint, domain.y).xy;


	

	posWS = mul(posWS, world);
	normWS = mul(normWS, world);
	
	pos = mul(pos, world);




	pos = mul(pos, view);
	pos = mul(pos, projection);

	output.pos = pos;
	output.posWS = posWS;
	output.normWS = normWS;
	output.uv = uv;

	return output;
}
