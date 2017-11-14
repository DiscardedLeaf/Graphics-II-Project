cbuffer perObject : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	matrix inverseTransposeWorld;
}



struct GS_Output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

struct GS_Input
{
	float4 pos : SV_POSITION;
};

[maxvertexcount(4)]
void main(
	point GS_Input input[1],
	inout TriangleStream< GS_Output > output
)
{
	GS_Output v1, v2, v3, v4;
	

	//positions will be in view space
	v1.pos = input[0].pos;
	v2.pos = float4(input[0].pos.x, input[0].pos.y + 1, input[0].pos.z, input[0].pos.w);
	v3.pos = float4(input[0].pos.x + 1, input[0].pos.y + 1, input[0].pos.z, input[0].pos.w);
	v4.pos = float4(input[0].pos.x + 1, input[0].pos.y, input[0].pos.z, input[0].pos.w);


	//put positions into projection space
	v1.pos = mul(v1.pos, projection);
	v2.pos = mul(v2.pos, projection);
	v3.pos = mul(v3.pos, projection);
	v4.pos = mul(v4.pos, projection);

	//give positions uv's for cloud texture
	v1.uv = float2(0.0f, 1.0f);
	v2.uv = float2(0.0f, 0.0f);
	v3.uv = float2(1.0f, 0.0f);
	v4.uv = float2(1.0f, 1.0f);

	output.Append(v1);
	output.Append(v2);
	output.Append(v3);
	output.Append(v4);

	output.RestartStrip();
}