struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 uv : UV;
	float3 norm : NORMAL;
};


float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 output = float4(input.uv, 1.0f);
	//TODO: lighting calculations for directional light
	return output;
}