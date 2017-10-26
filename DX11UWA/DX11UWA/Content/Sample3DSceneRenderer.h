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
		Microsoft::WRL::ComPtr<ID3D11InputLayout>   n_inputLayout; //position, uv's and normals
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout; //position and uv's

		//Direct3D vertex shaders
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader; //position and uv's
		Microsoft::WRL::ComPtr<ID3D11VertexShader>  n_vertexShader; //position, uv's and normals

		//Direct3D pixel shaders
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader; //position and uv's
		Microsoft::WRL::ComPtr<ID3D11PixelShader>   n_pixelShader; //position, uv's and normals

		//Direct3D constant buffers
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

		// Cube Resources
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		uint32	m_indexCount;

		//Lower Plane Resources
		Microsoft::WRL::ComPtr<ID3D11Buffer>        g_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		g_indexBuffer;
		ModelViewProjectionConstantBuffer	g_constantBufferData;
		uint32  g_indexCount;

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

