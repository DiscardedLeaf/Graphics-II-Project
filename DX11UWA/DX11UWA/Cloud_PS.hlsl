Texture2D Texture : register(t0);
sampler Sampler : register(s0);

struct PS_Input
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 main(PS_Input input) : SV_TARGET
{
	float4 color = Texture.Sample(Sampler, input.uv);
	if (color.w < .01)
		discard;
	return color;
}