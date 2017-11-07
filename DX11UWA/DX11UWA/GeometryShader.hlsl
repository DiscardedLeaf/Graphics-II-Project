cbuffer PerObject : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	matrix inverseTransposeWorld;
};

struct GSOutput
{
	float4 posWS : TEXCOORD1;
	float3 normWS : TEXCOORD2;
	float2 uv : TEXCOORD0;
	float4 pos : SV_POSITION;
};

struct GSInput
{
	float4 posWS : TEXCOORD1;
	float3 normWS : TEXCOORD2;
	float2 uv : TEXCOORD0;
	float4 pos : SV_POSITION;
};

[maxvertexcount(6)]
void main(
	point GSInput input[1],
	inout TriangleStream< GSOutput > output
)
{
	//variables needed for creating a double sided lighted triangle
	GSOutput v1, v2, v3, v4, v5, v6;
	float4 tempPos;
	float2 tempUV;
	float2 reverseUV = float2(1 - input[0].uv.x, input[0].uv.y); //flipped uv coordinates
	//triangle 1 vertex 1
	v1.pos = input[0].pos;
	v1.posWS = input[0].posWS;
	v1.normWS = input[0].normWS;
	v1.uv = input[0].uv; //set the uv coordinate to the bottom right (input should have the uv be 0, 1)
	
	//triangle 1 vertex 2
	tempPos = input[0].posWS;
	tempPos.y += 1; //sets the world space position to equal the normal position + 1
	tempUV = input[0].uv; 
	tempUV.y -= 1; //set the uv coordinate to the top left
	v2.pos = mul(mul(tempPos, view), projection); //make the position equal to the new world space projected position
	v2.posWS = tempPos;
	v2.normWS = input[0].normWS;
	v2.uv = tempUV;

	//triangle 1 vertex 3
	tempPos = input[0].posWS;
	tempPos.x += 1;
	tempUV = input[0].uv;
	tempUV.x += 1;
	v3.pos = mul(mul(tempPos, view), projection);
	v3.posWS = tempPos;
	v3.normWS = input[0].normWS;
	v3.uv = tempUV;

	//triangle 2 vertex 1 
	v4.pos = input[0].pos; //same position as v1 
	v4.posWS = input[0].posWS; 
	v4.normWS = -input[0].normWS; //normal is reversed for lighting 
	v4.uv = reverseUV; 

	//triangle 2 vertex 2
	tempPos = input[0].posWS;
	tempPos.x += 1; //same position as vertex v3 (so it gets drawn in the opposite order and appears while the camera is on both sides)
	tempUV = reverseUV;
	tempUV.x -= 1; //subtract x this time since the uv is flipped
	v5.pos = mul(mul(tempPos, view), projection);
	v5.posWS = tempPos;
	v5.normWS = -input[0].normWS;
	v5.uv = tempUV;

	//triangle 2 vertex 3
	tempPos = input[0].posWS;
	tempPos.y += 1;
	tempUV = reverseUV;
	tempUV.y -= 1;
	v6.pos = mul(mul(tempPos, view), projection);
	v6.posWS = tempPos;
	v6.normWS = -input[0].normWS;
	v6.uv = tempUV;

	output.Append(v1);
	output.Append(v2);
	output.Append(v3);
	output.Append(v4);
	output.Append(v5);
	output.Append(v6);
}