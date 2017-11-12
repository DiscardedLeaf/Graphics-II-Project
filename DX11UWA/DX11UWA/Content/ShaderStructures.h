#pragma once
#define MAX_LIGHTS 8
#define PI 3.14159265359

namespace DX11UWA
{
	//NOTE: use floats and ints for structures, other data types can screw with stuff

	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	//constant buffer to send lighting specific matrices to vertex shader
	struct PerObjectBuffer
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
		DirectX::XMFLOAT4X4 inverseTransposeWorld;
	};
	struct _Material
	{
		DirectX::XMFLOAT4 Emissive; //emissive light component of material
		DirectX::XMFLOAT4 Ambient; //ambient light component of material
		DirectX::XMFLOAT4 Diffuse; //diffuse light component of material
		DirectX::XMFLOAT4 Specular; //specular light component of material

		float SpecularPower; //how bright the specular highlight is
		bool useTexture; //whether or not to use a texture or simply the material color
		DirectX::XMFLOAT2 Padding; //keeps memory usage at a multiple of 16
	};

	struct MaterialProperties
	{
		_Material material;
	};

	struct Light
	{
		DirectX::XMFLOAT4 Position; //position of the light source
		DirectX::XMFLOAT4 Direction; //where the light is pointing
		DirectX::XMFLOAT4 Color; //color of the light source

		float SpotAngle; //angle in radians of spotlight cone
		float Radius;

		unsigned int LightType; //0 - Directional, 1 - Point, 2 - Spotlight
		float Enabled; //1 - enabled, 0 - disabled
		float useQuadraticAttenuation; 
		DirectX::XMFLOAT3 padding; //keeps the memory usage at a multiple of 16
	};

	struct LightProperties
	{
		DirectX::XMFLOAT4 CameraPosition; //position of the view matrix
		DirectX::XMFLOAT4 GlobalAmbient; //measure of ambient light applied to all objects in the scene
		Light Lights[MAX_LIGHTS];  //a container with all lights in the scene
	};

	struct TextureData //the plane beneath the mountain
	{
		uint32 texWidth;
		uint32 texHeight;
		DirectX::XMFLOAT2 padding;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};

	struct VertexPositionUVNormal
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 uv;
		DirectX::XMFLOAT3 normal;
	};
}