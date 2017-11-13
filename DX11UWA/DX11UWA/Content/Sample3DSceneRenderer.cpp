#include "pch.h"
#include "Sample3DSceneRenderer.h"
#include <fstream>
#include "..\Common\DirectXHelper.h"
#include "Common\DDSTextureLoader.h"

using namespace DX11UWA;
using namespace std;
using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	memset(m_kbuttons, 0, sizeof(m_kbuttons));
	m_currMousePos = nullptr;
	m_prevMousePos = nullptr;
	memset(&m_camera, 0, sizeof(XMFLOAT4X4));

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

void Sample3DSceneRenderer::LoadObjFile(const char * path,
										Microsoft::WRL::ComPtr<ID3D11Buffer>& vertexBuffer,
										Microsoft::WRL::ComPtr<ID3D11Buffer>& indexBuffer,
										uint32 & indexCount)
{
	vector<XMFLOAT3> positions;
	vector<XMFLOAT3> uvs;
	vector<XMFLOAT3> normals;
	vector<int> vertexIndices, uvIndices, normalIndices;
	char buffer[360];

	FILE * file = fopen(path, "r");

	while (true)
	{
		if (!file)
			return;
		int letter = fscanf(file, "%s", buffer, 360);
		if (letter == EOF)
			break;


		if (strcmp(buffer, "v") == 0)
		{
			XMFLOAT3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			positions.push_back(vertex);
		}
		else if (strcmp(buffer, "vt") == 0)
		{
			XMFLOAT3 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = 1 - uv.y;
			uv.z = 0.0f;
			uvs.push_back(uv);
		}
		else if (strcmp(buffer, "vn") == 0)
		{
			XMFLOAT3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			normals.push_back(normal);
		}
		else if (strcmp(buffer, "f") == 0)
		{
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);

			vertexIndices.push_back(vertexIndex[0] - 1);
			vertexIndices.push_back(vertexIndex[1] - 1);
			vertexIndices.push_back(vertexIndex[2] - 1);
			uvIndices.push_back(uvIndex[0] - 1);
			uvIndices.push_back(uvIndex[1] - 1);
			uvIndices.push_back(uvIndex[2] - 1);
			normalIndices.push_back(normalIndex[0] - 1);
			normalIndices.push_back(normalIndex[1] - 1);
			normalIndices.push_back(normalIndex[2] - 1);

			indexCount+=3;
		}
	}
	vector<VertexPositionUVNormal> objVertices;
	vector<unsigned short> objIndices;
	bool unique;
	for (int i = 0; i < indexCount; ++i)
	{
		unique = true;
		VertexPositionUVNormal vert;
		vert.pos = positions[vertexIndices[i]];
		vert.normal = normals[normalIndices[i]];
		vert.uv = uvs[uvIndices[i]];
		for (unsigned short j = 0; j < objVertices.size(); ++j)
		{
			if (vert.pos.x == objVertices[j].pos.x && vert.pos.y == objVertices[j].pos.y && vert.pos.z == objVertices[j].pos.z &&
				vert.normal.x == objVertices[j].normal.x && vert.normal.y == objVertices[j].normal.y && vert.normal.z == objVertices[j].normal.z &&
				vert.uv.x == objVertices[j].uv.x && vert.uv.y == objVertices[j].uv.y)
			{
				unique = false;
				objIndices.push_back(j);
				break;
			}
		}
		if (unique)
		{
			objVertices.push_back(vert);
			objIndices.push_back(objVertices.size() - 1);
		}
	}



	D3D11_SUBRESOURCE_DATA vertexBufferData;
	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = &objVertices[0];
	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(objVertices[0]) * objVertices.size(), D3D11_BIND_VERTEX_BUFFER);
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer));

	D3D11_SUBRESOURCE_DATA indexBufferData;
	ZeroMemory(&indexBufferData, sizeof(indexBufferData));
	indexBufferData.pSysMem = &objIndices[0];
	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(objIndices[0]) * objIndices.size(), D3D11_BIND_INDEX_BUFFER);
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &indexBuffer));

}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources(void)
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;
	camNearPlane = .01f;
	camFarPlane = 200.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, camNearPlane, camFarPlane);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
	XMStoreFloat4x4(&g_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
	XMStoreFloat4x4(&pDeath_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
	XMStoreFloat4x4(&geo_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
	XMStoreFloat4x4(&tamriel_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));



	// Eye is at (0,0.7,10.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 2.7f, -30.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_camera, XMMatrixInverse(nullptr, XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&g_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&pDeath_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&geo_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&tamriel_constantBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));

}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}


	// Update or move camera here
	UpdateCamera(timer, 6.0f, 0.25f);

}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
	XMStoreFloat4x4(&pDeath_constantBufferData.world, XMMatrixTranspose(XMMatrixMultiply(XMMatrixRotationY(radians), XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.0f, 0.0f), XMMatrixScaling(.25f, .25f, .25f)))));
	XMStoreFloat4x4(&pDeath_constantBufferData.inverseTransposeWorld, XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&pDeath_constantBufferData.world))));
}

void Sample3DSceneRenderer::UpdateCamera(DX::StepTimer const& timer, float const moveSpd, float const rotSpd)
{
	const float delta_time = (float)timer.GetElapsedSeconds();
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	if (m_kbuttons['W'])
	{
		XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, moveSpd * delta_time);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['S'])
	{
		XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, -moveSpd * delta_time);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['A'])
	{
		XMMATRIX translation = XMMatrixTranslation(-moveSpd * delta_time, 0.0f, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['D'])
	{
		XMMATRIX translation = XMMatrixTranslation(moveSpd * delta_time, 0.0f, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['X'])
	{
		XMMATRIX translation = XMMatrixTranslation( 0.0f, -moveSpd * delta_time, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons[VK_SPACE])
	{
		XMMATRIX translation = XMMatrixTranslation( 0.0f, moveSpd * delta_time, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['N']) //lowers the near plane
	{
		camNearPlane -= 10.f * delta_time;
		if (camNearPlane < .01f)
			camNearPlane = .01f;
		XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, camNearPlane, camFarPlane);

		XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

		XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

		XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&g_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&pDeath_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&geo_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&tamriel_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));

	}
	if (m_kbuttons['J']) //increases the near plane
	{
		camNearPlane += 10.f * delta_time;
		if (camNearPlane > 1000.f)
			camNearPlane = 1000.f;
		XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, camNearPlane, camFarPlane);

		XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

		XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

		XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&g_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&pDeath_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&geo_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&tamriel_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));


	}
	if (m_kbuttons['M']) //lowers the far plane (it'll take a while before you notice it)
	{
		camFarPlane -= 10.f * delta_time;
		if (camFarPlane < camNearPlane)
			camFarPlane = camNearPlane + .01f;
		XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, camNearPlane, camFarPlane);

		XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

		XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

		XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&g_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&pDeath_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&geo_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&tamriel_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));

	}
	if (m_kbuttons['K']) //increases the far plane
	{
		camFarPlane += 10.f * delta_time;
		if (camFarPlane > 1000000.f)
			camFarPlane = 1000000.f;
		XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, camNearPlane, camFarPlane);

		XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

		XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

		XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&g_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&pDeath_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&geo_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));
		XMStoreFloat4x4(&tamriel_constantBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));

	}
	if (m_kbuttons['U'] & 0x1) //toggles wireframe mode
	{
		if (wireframeButtonDownTime <= 0)
		{
			if (renderInWireframe)
			{
				m_deviceResources->GetD3DDeviceContext()->RSSetState(m_rasterizerState.Get());
				renderInWireframe = !renderInWireframe;
			}
			else
			{
				m_deviceResources->GetD3DDeviceContext()->RSSetState(w_rasterizerState.Get());
				renderInWireframe = !renderInWireframe;
			}
			wireframeButtonDownTime = .1f;
		}
	}

	if (m_currMousePos) 
	{
		if (m_currMousePos->Properties->IsRightButtonPressed && m_prevMousePos)
		{
			float dx = m_currMousePos->Position.X - m_prevMousePos->Position.X;
			float dy = m_currMousePos->Position.Y - m_prevMousePos->Position.Y;

			XMFLOAT4 pos = XMFLOAT4(m_camera._41, m_camera._42, m_camera._43, m_camera._44);

			m_camera._41 = 0;
			m_camera._42 = 0;
			m_camera._43 = 0;

			XMMATRIX rotX = XMMatrixRotationX(dy * rotSpd * delta_time);
			XMMATRIX rotY = XMMatrixRotationY(dx * rotSpd * delta_time);

			XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
			temp_camera = XMMatrixMultiply(rotX, temp_camera);
			temp_camera = XMMatrixMultiply(temp_camera, rotY);

			XMStoreFloat4x4(&m_camera, temp_camera);

			m_camera._41 = pos.x;
			m_camera._42 = pos.y;
			m_camera._43 = pos.z;
		}
		m_prevMousePos = m_currMousePos;
	}


}

void Sample3DSceneRenderer::SetKeyboardButtons(const char* list)
{
	memcpy_s(m_kbuttons, sizeof(m_kbuttons), list, sizeof(m_kbuttons));
}

void Sample3DSceneRenderer::SetMousePosition(const Windows::UI::Input::PointerPoint^ pos)
{
	m_currMousePos = const_cast<Windows::UI::Input::PointerPoint^>(pos);
}

void Sample3DSceneRenderer::SetInputDeviceData(const char* kb, const Windows::UI::Input::PointerPoint^ pos)
{
	SetKeyboardButtons(kb);
	SetMousePosition(pos);
}

void DX11UWA::Sample3DSceneRenderer::StartTracking(void)
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking(void)
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render(void)
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();
	

	//decrement all button timers by a little bit
	wireframeButtonDownTime -= .01;




	//XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));

	//// Prepare the constant buffer to send it to the graphics device.
	//context->UpdateSubresource1(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0, 0);
	//// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	//context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	//// Each index is one 16-bit unsigned integer (short).
	//context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//context->IASetInputLayout(m_inputLayout.Get());
	//// Attach our vertex shader.
	//context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	//// Send the constant buffer to the graphics device.
	//context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr);
	//// Attach our pixel shader.
	//context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	//// Draw the cube
	//context->DrawIndexed(m_indexCount, 0, 0);



	//--------------------------------------------------------------------------------------------------------------------------

	//lighting stuff
	//rotate the directional light
	//XMStoreFloat4(&m_lighting.Lights[0].Direction, XMVector4Transform(XMLoadFloat4(&m_lighting.Lights[0].Direction), XMMatrixRotationX(-.01f)));
	//move the point light
	XMStoreFloat4(&m_lighting.Lights[1].Position, XMVector4Transform(XMLoadFloat4(&m_lighting.Lights[1].Position), XMMatrixRotationY(-.05f)));
	//move the spot light and change its direction
	XMStoreFloat4(&m_lighting.Lights[2].Direction, XMVector4Transform(XMLoadFloat4(&m_lighting.Lights[2].Direction), XMMatrixRotationZ(-.05f)));
	XMStoreFloat4(&m_lighting.Lights[2].Position, XMVector4Transform(XMLoadFloat4(&m_lighting.Lights[2].Position), XMMatrixRotationY(-.03f)));
	//store the camera's new position
	XMFLOAT4 cameraPosition = { m_camera._41, m_camera._42, m_camera._43, m_camera._44 };
	XMStoreFloat4(&m_lighting.CameraPosition, XMLoadFloat4(&cameraPosition));
	XMStoreFloat4(&tamriel_cameraPosition.cameraPosition, XMLoadFloat4(&cameraPosition));

	//floor
	XMStoreFloat4x4(&g_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
	//Set constant buffer's world matrix to plane's world matrix
	context->UpdateSubresource1(n_constantBuffer.Get(), 0, NULL, &g_constantBufferData, 0, 0, 0);
	context->UpdateSubresource1(t_constantBuffer.Get(), 0, NULL, &g_materialProperties, 0, 0, 0);
	context->UpdateSubresource1(l_constantBuffer.Get(), 0, NULL, &m_lighting, 0, 0, 0);

	//set vertex buffer to plane
	stride = sizeof(VertexPositionUVNormal);
	context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(n_inputLayout.Get());

	context->VSSetShader(n_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, n_constantBuffer.GetAddressOf(), nullptr, nullptr);

	context->PSSetShader(nDir_pixelShader.Get(), nullptr, 0);
	context->PSSetConstantBuffers1(0, 1, t_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetConstantBuffers1(1, 1, l_constantBuffer.GetAddressOf(), nullptr, nullptr);
	
	//context->DrawIndexed(g_indexCount, 0, 0);


	//----------------------------------------------------------------------------------------------------------------------------------
	//dead penguin

	XMStoreFloat4x4(&pDeath_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
	stride = sizeof(VertexPositionUVNormal);

	context->UpdateSubresource1(n_constantBuffer.Get(), 0, NULL, &pDeath_constantBufferData, 0, 0, 0);
	context->UpdateSubresource1(t_constantBuffer.Get(), 0, NULL, &pDeath_materialProperties, 0, 0, 0);
	context->UpdateSubresource1(l_constantBuffer.Get(), 0, NULL, &m_lighting, 0, 0, 0);

	context->IASetVertexBuffers(0, 1, pDeath_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(pDeath_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(n_inputLayout.Get());

	context->VSSetShader(n_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, n_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetShader(nDir_pixelShader.Get(), nullptr, 0);
	context->PSSetConstantBuffers1(0, 1, t_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetConstantBuffers1(1, 1, l_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
	context->PSSetShaderResources(0, 1, p_ShaderResourceView.GetAddressOf());
	
	//context->DrawIndexed(pDeath_indexCount, 0, 0);

	//geoShader objects

	XMStoreFloat4x4(&geo_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
	stride = sizeof(VertexPositionUVNormal);

	context->UpdateSubresource1(n_constantBuffer.Get(), 0, NULL, &geo_constantBufferData, 0, 0, 0);
	context->UpdateSubresource1(t_constantBuffer.Get(), 0, NULL, &geo_materialProperties, 0, 0, 0);
	context->UpdateSubresource1(l_constantBuffer.Get(), 0, NULL, &m_lighting, 0, 0, 0);

	context->IASetVertexBuffers(0, 1, geo_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(geo_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->IASetInputLayout(n_inputLayout.Get());

	context->VSSetShader(n_vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers1(0, 1, n_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->GSSetShader(n_geometryShader.Get(), nullptr, 0);
	context->GSSetConstantBuffers1(0, 1, n_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetShader(nDir_pixelShader.Get(), nullptr, 0);
	context->PSSetConstantBuffers1(0, 1, t_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetConstantBuffers1(1, 1, l_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
	context->PSSetShaderResources(0, 1, geo_ShaderResourceView.GetAddressOf());

	//context->DrawIndexed(geo_indexCount, 0, 0);

	//set the geometry shader to null so it doesnt screw the other draws up
	context->GSSetShader(nullptr, nullptr, 0);

	//------------------------------------------------------------------------------------------------------------------
	//terrain
	XMStoreFloat4x4(&tamriel_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));
	stride = sizeof(VertexPositionUVNormal);

	context->UpdateSubresource1(n_constantBuffer.Get(), 0, NULL, &tamriel_constantBufferData, 0, 0, 0);
	context->UpdateSubresource1(t_constantBuffer.Get(), 0, NULL, &tamriel_materialProperties, 0, 0, 0);
	context->UpdateSubresource1(l_constantBuffer.Get(), 0, NULL, &m_lighting, 0, 0, 0);
	context->UpdateSubresource1(h_constantBuffer.Get(), 0, NULL, &tamriel_textureData, 0, 0, 0);

	context->IASetVertexBuffers(0, 1, tamriel_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(tamriel_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
	context->IASetInputLayout(n_inputLayout.Get());

	context->VSSetShader(t_vertexShader.Get(), nullptr, 0);
	context->HSSetShader(t_hullShader.Get(), nullptr, 0);
	context->HSSetConstantBuffers(0, 1, cp_constantBuffer.GetAddressOf());
	context->DSSetShader(t_domainShader.Get(), nullptr, 0);
	context->DSSetConstantBuffers(0, 1, n_constantBuffer.GetAddressOf());
	context->PSSetShader(nDir_pixelShader.Get(), nullptr, 0);
	context->PSSetConstantBuffers1(0, 1, t_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetConstantBuffers1(1, 1, l_constantBuffer.GetAddressOf(), nullptr, nullptr);
	context->PSSetSamplers(0, 1, m_sampler.GetAddressOf());

	context->DrawIndexed(tamriel_indexCount, 0, 0);

	//set hull and domain shader to null so it doesnt screw with other draws
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);
}

void Sample3DSceneRenderer::CreateDeviceDependentResources(void)
{
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	auto loadObjVSTask = DX::ReadDataAsync(L"ObjVertexShader.cso");
	auto loadObjDirPSTask = DX::ReadDataAsync(L"ObjDirectionalPixelShader.cso");
	auto loadGSTask = DX::ReadDataAsync(L"GeometryShader.cso");
	auto loadTerrain_VSTask = DX::ReadDataAsync(L"Terrain_VS.cso");
	auto loadTerrain_HSTask = DX::ReadDataAsync(L"Terrain_HS.cso");
	auto loadTerrain_DSTask = DX::ReadDataAsync(L"Terrain_DS.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_vertexShader));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &m_inputLayout));
	});

	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_pixelShader));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffer));
	});

	//After the obj vertex shader file is loaded, create the shader and input layout
	auto createObjVSTask = loadObjVSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &n_vertexShader));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &n_inputLayout));
	});

	//After the obj directional pixel shader file is loaded, create the shader and constant buffers
	auto createObjDirPSTask = loadObjDirPSTask.then([this](const std::vector<byte>& fileData)
	{
		//create the pixel shader
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &nDir_pixelShader));

		//create the constant buffers
		CD3D11_BUFFER_DESC n_constantBufferDesc(sizeof(PerObjectBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&n_constantBufferDesc, nullptr, &n_constantBuffer));
		CD3D11_BUFFER_DESC t_constantBufferDesc(sizeof(MaterialProperties), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&t_constantBufferDesc, nullptr, &t_constantBuffer));
		CD3D11_BUFFER_DESC l_constantBufferDesc(sizeof(LightProperties), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&l_constantBufferDesc, nullptr, &l_constantBuffer));

		//create sampler object
		D3D11_SAMPLER_DESC m_sampler_desc;
		m_sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
		m_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		m_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		m_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		m_sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		m_sampler_desc.BorderColor[0] = 1.0f;
		m_sampler_desc.BorderColor[1] = 1.0f;
		m_sampler_desc.BorderColor[2] = 1.0f;
		m_sampler_desc.BorderColor[3] = 1.0f;
		m_sampler_desc.MaxAnisotropy = 1;
		m_sampler_desc.MipLODBias = 0;
		m_sampler_desc.MaxLOD = FLT_MAX;
		m_sampler_desc.MinLOD = -FLT_MAX;
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&m_sampler_desc, &m_sampler));

	});

	//create the 3 shaders necessary for the terrain and any variables that they need
	auto createTerrain_VSTask = loadTerrain_VSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &t_vertexShader));
		CD3D11_BUFFER_DESC h_constantBufferDesc(sizeof(TextureData), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&h_constantBufferDesc, nullptr, &h_constantBuffer));

		D3D11_RASTERIZER_DESC fillRasterizerDesc;
		ZeroMemory(&fillRasterizerDesc, sizeof(fillRasterizerDesc));
		fillRasterizerDesc.CullMode = D3D11_CULL_BACK;
		fillRasterizerDesc.FillMode = D3D11_FILL_SOLID;
		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&fillRasterizerDesc, &m_rasterizerState);

		D3D11_RASTERIZER_DESC wireframeRasterizerDesc;
		ZeroMemory(&wireframeRasterizerDesc, sizeof(wireframeRasterizerDesc));
		wireframeRasterizerDesc.CullMode = D3D11_CULL_NONE;
		wireframeRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&wireframeRasterizerDesc, &w_rasterizerState);

		renderInWireframe = false;

	});
	auto createTerrain_HSTask = loadTerrain_HSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateHullShader(&fileData[0], fileData.size(), nullptr, &t_hullShader));
		CD3D11_BUFFER_DESC cp_constantBufferDesc(sizeof(CameraPosition), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&cp_constantBufferDesc, nullptr, &cp_constantBuffer));

	});
	auto createTerrain_DSTask = loadTerrain_DSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateDomainShader(&fileData[0], fileData.size(), nullptr, &t_domainShader));
	});

	auto createGeoShaderTask = loadGSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateGeometryShader(&fileData[0], fileData.size(), nullptr, &n_geometryShader));
	});

	// Once both shaders are loaded, create the mesh.
	auto createCubeTask = (createPSTask && createVSTask).then([this]()
	{
		// Load mesh vertices. Each vertex has a position and a color.
		static const VertexPositionColor cubeVertices[] =
		{
			{XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f)},
			{XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f)},
			{XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f)},
			{XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f)},
			{XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f)},
			{XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f)},
			{XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f)},
			{XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
		};

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = cubeVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_vertexBuffer));

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.
		static const unsigned short cubeIndices[] =
		{
			0,1,2, // -x
			1,3,2,

			4,6,5, // +x
			5,6,7,

			0,5,1, // -y
			0,4,5,

			2,7,6, // +y
			2,3,7,

			0,6,4, // -z
			0,2,6,

			1,7,3, // +z
			1,5,7,
		};

		m_indexCount = ARRAYSIZE(cubeIndices);

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = cubeIndices;
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_indexBuffer));
	});

	//obsidian floor
	auto createFloorTask = (createObjDirPSTask && createObjVSTask).then([this]()
	{
		static VertexPositionUVNormal gridVertices[] = 
		{
			{XMFLOAT3(-1.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f,  1.0f, 0.0f)},
			{XMFLOAT3(-1.0f, 0.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f,  1.0f, 0.0f)},
			{XMFLOAT3( 1.0f, 0.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f,  1.0f, 0.0f)},
			{XMFLOAT3( 1.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f,  1.0f, 0.0f)},
			{XMFLOAT3(-1.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)},
			{XMFLOAT3(-1.0f, 0.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)},
			{XMFLOAT3( 1.0f, 0.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)},
			{XMFLOAT3( 1.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)}
		};

		D3D11_SUBRESOURCE_DATA vertexData;
		ZeroMemory(&vertexData, sizeof(vertexData));
		vertexData.pSysMem = gridVertices;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(gridVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &g_vertexBuffer));

		static const unsigned short gridIndices[] =
		{
			0,1,2, //top of plane
			0,2,3,

			6,5,4, //bottom of plane
			7,6,4
		};

		g_indexCount = ARRAYSIZE(gridIndices);
		D3D11_SUBRESOURCE_DATA indexData;
		ZeroMemory(&indexData, sizeof(indexData));
		indexData.pSysMem = gridIndices;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(gridIndices), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexData, &g_indexBuffer));

		_Material obsidian;
		obsidian.Ambient = { .05375f, .05f, .06625f, 1.0f };
		obsidian.Diffuse = { .18275f, .17f, .22525f, 1.0f };
		obsidian.Emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
		obsidian.Specular = { .332741f, .328634f, .346435f, 1.0f };
		obsidian.SpecularPower = 38.4f;
		obsidian.useTexture = false;
		g_materialProperties.material = obsidian;

		XMStoreFloat4x4(&g_constantBufferData.world, XMMatrixTranspose(XMMatrixMultiply(XMMatrixTranslation(0, -.51f, 0), XMMatrixScaling(10, 1, 10))));
		XMStoreFloat4x4(&g_constantBufferData.inverseTransposeWorld, XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&g_constantBufferData.world))));
	});

	//dead penguin
	auto createPencassoDeathTask = (createObjVSTask && createObjDirPSTask).then([this]()
	{
		//load vertex data
		LoadObjFile("Assets/PencassoDeath.obj", pDeath_vertexBuffer, pDeath_indexBuffer, pDeath_indexCount);

		//create world and inversetransposeworld matrices
		XMStoreFloat4x4(&pDeath_constantBufferData.world, XMMatrixTranspose(XMMatrixMultiply(XMMatrixTranslation(0.0f, 0.0f, 0.0f),XMMatrixScaling(.25f, .25f, .25f))));
		XMStoreFloat4x4(&pDeath_constantBufferData.inverseTransposeWorld, XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&pDeath_constantBufferData.world))));

		//create a material to use for the dead penguin and set the constant buffer data to use it
		_Material deadPenguin;
		deadPenguin.Ambient = { .25f, .25f, .25f, .25f };
		deadPenguin.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
		deadPenguin.Emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
		deadPenguin.Specular = { 1.0f, 1.0f, 1.0f, 1.0f };
		deadPenguin.SpecularPower = 32;
		deadPenguin.useTexture = true;
		pDeath_materialProperties.material = deadPenguin;

		//use dds texture file creater to make the pencasso shaderResourceView
		DX::ThrowIfFailed(CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets/peng.dds", nullptr, &p_ShaderResourceView));
	});

	//lights
	auto createLights = (createObjDirPSTask).then([this]()
	{
		//initialize all lights to disabled
		for (int i = 0; i < MAX_LIGHTS; ++i)
			m_lighting.Lights[i].Enabled = 0;

		//directional light (the sun)
		Light hereComesTheSun;
		hereComesTheSun.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		hereComesTheSun.Direction = { 0.0f, -1.0f, 0.0f, 0.0f };
		hereComesTheSun.Enabled = 1;
		hereComesTheSun.LightType = 0;

		//point light
		Light star;
		star.Color = { 0.0f, 1.0f, 0.0f, 1.0f };
		star.Position = { 5.0f, 0.0f, 0.0f, 1.0f };
		star.LightType = 1;
		star.Enabled = 1;
		star.useQuadraticAttenuation = 1;
		star.Radius = 10;

		//spot light
		Light flashLight;
		flashLight.Color = { 1.0f, 0.0f, 0.0f, 1.0f };
		flashLight.Direction = { .1f, -1.0f, 0.0f, 0.0f };
		flashLight.Enabled = 1;
		flashLight.LightType = 2;
		flashLight.Position = { 4.0f, 3.0f, 0.0f, 1.0f };
		flashLight.SpotAngle = PI *.125f;
		flashLight.useQuadraticAttenuation = 1;
		
		//add all created lights to the list
		m_lighting.Lights[0] = hereComesTheSun;
		m_lighting.Lights[1] = star;
		m_lighting.Lights[2] = flashLight;
		m_lighting.GlobalAmbient = { 0.25f, 0.25f, 0.25f, 0.25f };
	});
	
	//floating squares
	auto createGeoShaderStuff = createGeoShaderTask.then([this]()
	{
		static vector<VertexPositionUVNormal> geoPoints;
		static vector<unsigned short> geoIndices;
		for (int i = 0; i < 50; ++i)
		{
			VertexPositionUVNormal point;
			float x, z;
			x = rand() % 1000;
			x = (x / 500.0f) - 1.0f;
			z = rand() % 1000;
			z = (z / 500.0f) - 1.0f;
			point.pos = XMFLOAT3(x, 0.0f, z);
			point.normal = XMFLOAT3(0.0f, 0.0f, 1.0f);
			point.uv = XMFLOAT3(0.0f, 1.0f, 0.0f);
			geoPoints.push_back(point);
			geoIndices.push_back(i);
		}
		geo_indexCount = geoIndices.size();

		D3D11_SUBRESOURCE_DATA vertexData;
		ZeroMemory(&vertexData, sizeof(vertexData));
		vertexData.pSysMem = &geoPoints[0];

		D3D11_BUFFER_DESC vertexBufferDesc;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.ByteWidth = sizeof(geoPoints[0]) * geoPoints.size();
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = sizeof(geoPoints[0]);
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &geo_vertexBuffer));

		D3D11_SUBRESOURCE_DATA indexData;
		ZeroMemory(&indexData, sizeof(indexData));
		indexData.pSysMem = &geoIndices[0];

		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.ByteWidth = sizeof(geoIndices[0]) * geoIndices.size();
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = sizeof(geoIndices[0]);
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexData, &geo_indexBuffer));

		_Material ruby;
		ruby.Ambient = { .1745f, .01175f, 0.1175f, 1.0f };
		ruby.Diffuse = {.61424f, .04136f, .04136f, 1.0f};
		ruby.Emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
		ruby.Specular = { .727811f, .626959f, .626959f, 1.0f };
		ruby.SpecularPower = 76.8f;
		ruby.useTexture = true;
		geo_materialProperties.material = ruby;

		XMStoreFloat4x4(&geo_constantBufferData.world, XMMatrixTranspose(XMMatrixMultiply(XMMatrixTranslation(0.0, -3.0f, 0.0f),XMMatrixScaling(10.0f, 0.0f, 10.0f))));
		XMStoreFloat4x4(&geo_constantBufferData.inverseTransposeWorld, XMMatrixInverse(nullptr, XMMatrixTranspose(XMLoadFloat4x4(&geo_constantBufferData.world))));

		DX::ThrowIfFailed(CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets/mScottTexture.dds", nullptr, &geo_ShaderResourceView));
	});
	//terrain
	auto createMountainTask = (createTerrain_VSTask && createTerrain_HSTask && createTerrain_DSTask && createObjDirPSTask).then([this]()
	{
		static vector<VertexPositionUVNormal> vertices;
		static vector<unsigned short> indices;
		int xCount = 0;
		int zCount = 0;

		for (float x = -1.f; x < 1.05f; x += .1f)
		{
			zCount = 0;
			for (float z = -1.f; z < 1.05f; z += .1f)
			{
				VertexPositionUVNormal vertex;
				vertex.pos.x = x;
				vertex.pos.y = 0.0f;
				vertex.pos.z = z;
				vertex.normal = { 0.0f, 1.0f, 0.0f };
				vertex.uv.x = (x + 1) * .5f;
				vertex.uv.y = (1 - z) * .5f;
				vertices.push_back(vertex);
				++zCount;
			}
			++xCount;
		}
		for (int x = 0; x < xCount - 1; ++x)
		{
			for (int z = 0; z < zCount - 1; ++z)
			{
				indices.push_back(x * zCount + z);
				indices.push_back(x * zCount + z + 1);
				indices.push_back(x * zCount + z + zCount);
				indices.push_back(x * zCount + z + zCount + 1);
			}
		}
		tamriel_indexCount = indices.size();

		D3D11_SUBRESOURCE_DATA vertexData;
		ZeroMemory(&vertexData, sizeof(vertexData));
		vertexData.pSysMem = &vertices[0];

		D3D11_BUFFER_DESC vertexBufferDesc;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.ByteWidth = sizeof(vertices[0]) * vertices.size();
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = sizeof(vertices[0]);
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &tamriel_vertexBuffer));

		D3D11_SUBRESOURCE_DATA indexData;
		ZeroMemory(&indexData, sizeof(indexData));
		indexData.pSysMem = &indices[0];

		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.ByteWidth = sizeof(indices[0]) * indices.size();
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = sizeof(indices[0]);
		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexData, &tamriel_indexBuffer));

		_Material rock;
		rock.Ambient = { 0.25f, 0.25f, 0.25f, 1.0f };
		rock.Diffuse = { 0.4f, 0.3f, .05f, 1.0f };
		rock.Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
		rock.Specular = { .1f, .1f, .1f, 1.0f };
		rock.SpecularPower = 8;
		rock.useTexture = false;
		tamriel_materialProperties.material = rock;

		tamriel_textureData.texHeight = 819;
		tamriel_textureData.texWidth = 1024;

		XMStoreFloat4x4(&tamriel_constantBufferData.world, XMMatrixTranspose(XMMatrixScaling(50, 0, 50)));
		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"Assets/tamrielHeightMap.dds", nullptr, &tamriel_ShaderResourceView);

	});


	// Once the objects are loaded, the objects are ready to be rendered.
	(createCubeTask &&
	 createFloorTask &&
	 createPencassoDeathTask &&
	 createLights &&
	 createGeoShaderStuff &&
	 createMountainTask).then([this]()
	{
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources(void)
{
	m_loadingComplete = false;
	m_rasterizerState.Reset();
	w_rasterizerState.Reset();
	m_vertexShader.Reset();
	n_vertexShader.Reset();
	t_vertexShader.Reset();
	t_hullShader.Reset();
	t_domainShader.Reset();
	n_geometryShader.Reset();
	m_inputLayout.Reset();
	n_inputLayout.Reset();
	m_pixelShader.Reset();
	nDir_pixelShader.Reset();
	m_constantBuffer.Reset();
	n_constantBuffer.Reset();
	t_constantBuffer.Reset();
	l_constantBuffer.Reset();
	h_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	g_vertexBuffer.Reset();
	pDeath_vertexBuffer.Reset();
	geo_vertexBuffer.Reset();
	geo_ShaderResourceView.Reset();
	p_ShaderResourceView.Reset();
	m_indexBuffer.Reset();
	g_indexBuffer.Reset();
	pDeath_indexBuffer.Reset();
	geo_indexBuffer.Reset();
}