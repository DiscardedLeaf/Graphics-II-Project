
cbuffer cameraPositionData : register(b0)
{
	float4 cameraPosition;
	float3 cameraDirection;
	float cameraAngleRadians;
	float maxViewDistance;
	float3 padding;
}

cbuffer perObject : register(b1)
{
	matrix world;
	matrix view;
	matrix projection;
	matrix inverseTransposeWorld;
}


// Input control point
struct VS_CONTROL_POINT_OUTPUT
{
	float3 pos : POSITION;
	float3 uv : TEXCOORD;
	float3 norm : NORMAL;
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
	// TODO: change/add other stuff
};

#define NUM_CONTROL_POINTS 4

//function to let me know if the square is in view of the camera
bool renderSquare(float4 squarePos)
{
	float minCos = cos(cameraAngleRadians);
	float cosAngle = dot(normalize(cameraDirection), -normalize(cameraPosition - squarePos));
	if (cosAngle < minCos)
		return false;
	return true;
}

int tesselationFactor(VS_CONTROL_POINT_OUTPUT a, VS_CONTROL_POINT_OUTPUT b)
{
	float3 lineMidpoint = lerp(a.pos, b.pos, .5f);
	lineMidpoint = mul(float4(lineMidpoint, 1.0f), world).xyz;
	float distance = length(cameraPosition.xyz - lineMidpoint);
	distance = distance / maxViewDistance;
	if (distance >= 1.0f)
		distance = .99f;

	distance = 1.0f - distance;
	return ((int)(distance * 32)) + 1;
}



// Patch Constant Function
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT output;

	//check if inside camera view
	float4 pos1 = float4(ip[0].pos, 1.0f);
	float4 pos2 = float4(ip[3].pos, 1.0f);

	pos1 = mul(pos1, world);
	pos2 = mul(pos2, world);

	float4 squarePos = float4((pos1.x + pos2.x) * .5f, (pos1.y + pos2.y) * .5f, (pos1.z + pos2.z) * .5f, 1.0f);

	if (renderSquare(squarePos))
	{
		output.EdgeTessFactor[0] = tesselationFactor(ip[0], ip[1]);
		output.EdgeTessFactor[1] = tesselationFactor(ip[1], ip[3]);
		output.EdgeTessFactor[2] = tesselationFactor(ip[3], ip[2]);
		output.EdgeTessFactor[3] = tesselationFactor(ip[2], ip[0]);
		output.InsideTessFactor[0] = 
		output.InsideTessFactor[1] = (int)((output.EdgeTessFactor[0] + output.EdgeTessFactor[1] + output.EdgeTessFactor[2] + output.EdgeTessFactor[3]) * .25f);
	}
	else
	{
		output.EdgeTessFactor[0] = 
		output.EdgeTessFactor[1] = 
		output.EdgeTessFactor[2] = 
		output.EdgeTessFactor[3] = 
		output.InsideTessFactor[0] = 
		output.InsideTessFactor[1] = 0;
	}

	//output.EdgeTessFactor[0] =
	//	output.EdgeTessFactor[1] =
	//	output.EdgeTessFactor[2] =
	//	output.EdgeTessFactor[3] =
	//	output.InsideTessFactor[0] =
	//	output.InsideTessFactor[1] = 2;

	return output;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main( 
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT output;

	// Insert code to compute Output here
	output.pos= ip[i].pos;
	output.uv = ip[i].uv;
	output.norm = ip[i].norm;

	return output;
}
