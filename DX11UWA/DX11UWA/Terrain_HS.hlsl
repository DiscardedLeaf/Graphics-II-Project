
cbuffer cameraPositionData : register(b0)
{
	float4 cameraPosition;
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

// Patch Constant Function
HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT output;

	// Insert code to compute Output here
	output.EdgeTessFactor[0] = 
		output.EdgeTessFactor[1] = 
		output.EdgeTessFactor[2] = 
		output.EdgeTessFactor[3] =
		output.InsideTessFactor[0] =
		output.InsideTessFactor[1] = 8; // e.g. could calculate dynamic tessellation factors instead

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
