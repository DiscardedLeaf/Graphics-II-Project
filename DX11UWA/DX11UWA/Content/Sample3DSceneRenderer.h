#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"


namespace DX11UWA
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void LoadObjFile(const char * path, Microsoft::WRL::ComPtr<ID3D11Buffer>& vertexBuffer, Microsoft::WRL::ComPtr<ID3D11Buffer>& indexBuffer, uint32& indexCount);
		void CreateDeviceDependentResources(void);
		void CreateWindowSizeDependentResources(void);
		void ReleaseDeviceDependentResources(void);
		void Update(DX::StepTimer const& timer);
		void Render(void);
		void StartTracking(void);
		void TrackingUpdate(float positionX);
		void StopTracking(void);
		inline bool IsTracking(void) { return m_tracking; }

		// Helper functions for keyboard and mouse input
		void SetKeyboardButtons(const char* list);
		void SetMousePosition(const Windows::UI::Input::PointerPoint^ pos);
		void SetInputDeviceData(const char* kb, const Windows::UI::Input::PointerPoint^ pos);


	private:
		void Rotate(float radians);
		void UpdateCamera(DX::StepTimer const& timer, float const moveSpd, float const rotSpd);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		//Direct3D input layouts
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			m_inputLayout;			 //position and uv's
		Microsoft::WRL::ComPtr<ID3D11InputLayout>			n_inputLayout;			 //position, uv's and normals

		//Direct3D vertex shaders
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			m_vertexShader;			 //position and uv's
		Microsoft::WRL::ComPtr<ID3D11VertexShader>			n_vertexShader;			 //position, uv's and normals

		//Direct3D pixel shaders
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			m_pixelShader;			 //position and uv's
		Microsoft::WRL::ComPtr<ID3D11PixelShader>			nDir_pixelShader;		 //position, uv's and normals for directional light

		//Direct3D geometry shaders
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>		n_geometryShader;		 //create geometry that can be lit and textured

		//Direct3D constant buffers
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_constantBuffer;		 //constant buffer for world objects
		Microsoft::WRL::ComPtr<ID3D11Buffer>				n_constantBuffer;	     //constant buffer for normalized world objects
		Microsoft::WRL::ComPtr<ID3D11Buffer>				t_constantBuffer;		 //constant buffer for texturing
		Microsoft::WRL::ComPtr<ID3D11Buffer>				l_constantBuffer;		 //constant buffer for lighting

		//Direct3D texture objects
		Microsoft::WRL::ComPtr<ID3D11Texture2D>				p_texture;				//texture object for pencasso
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	p_ShaderResourceView;	//shader resource view for the pencasso texture

		//Direct3D sampler objects
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			m_sampler;				//sampler object for texturing

		// Cube Resources
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				m_indexBuffer;
		ModelViewProjectionConstantBuffer					m_constantBufferData;
		uint32												m_indexCount;

		//Lower Plane Resources
		Microsoft::WRL::ComPtr<ID3D11Buffer>				g_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				g_indexBuffer;
		PerObjectBuffer										g_constantBufferData;
		uint32												g_indexCount;
		MaterialProperties									g_materialProperties;

		//Pencasso Resources
		Microsoft::WRL::ComPtr<ID3D11Buffer>				pDeath_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				pDeath_indexBuffer;
		PerObjectBuffer										pDeath_constantBufferData;
		uint32												pDeath_indexCount;
		MaterialProperties									pDeath_materialProperties;

		//first geoshader resources
		Microsoft::WRL::ComPtr<ID3D11Buffer>				geo_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				geo_indexBuffer;
		PerObjectBuffer										geo_constantBufferData;
		uint32												geo_indexCount;
		MaterialProperties									geo_materialProperties;

		//Light Objects
		LightProperties										m_lighting;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;

		// Data members for keyboard and mouse input
		char	m_kbuttons[256];
		Windows::UI::Input::PointerPoint^ m_currMousePos;
		Windows::UI::Input::PointerPoint^ m_prevMousePos;

		// Matrix data member for the camera
		DirectX::XMFLOAT4X4 m_camera;
	};
}

