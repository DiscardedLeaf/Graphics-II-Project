textureCUBE Texture : register(t0);
sampler Sampler : register(s0);

struct PS_Input
{
	float4 pos : SV_POSITION;
	float3 posLS : UV;
};

float4 main(PS_Input input) : SV_TARGET
{
	float4 color = Texture.Sample(Sampler, input.posLS);
	return color;
}