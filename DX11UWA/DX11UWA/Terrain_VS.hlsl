
struct VS_Input
{
	float3 pos : POSITION;
	float3 uv : TEXCOORD;
	float3 norm : NORMAL;
};
struct VS_Output
{
	float3 pos : POSITION;
	float3 uv : TEXCOORD;
	float3 norm : NORMAL;
};

VS_Output main(VS_Input input)
{
	VS_Output output;

	output.pos = input.pos;
	output.uv = input.uv;
	output.norm = input.norm;

	return output;
}