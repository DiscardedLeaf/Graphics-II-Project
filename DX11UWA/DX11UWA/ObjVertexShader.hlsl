cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};


float4 main( float4 pos : POSITION ) : SV_POSITION
{
	
	return pos;
}