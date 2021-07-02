#include <windows.h>
#include <atlbase.h>
#pragma warning( disable: 4996 )
#include <strsafe.h>
#pragma warning( default: 4996 )

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "DXUT.h"
#include "Renderer.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"

#include <ScreenGrab.h>

#define WATER_TERRAIN_FILE L"TestIsland.bmp"

D3D11_INPUT_ELEMENT_DESC Renderer::SimpleVertexElements[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
int Renderer::SimpleVertexElementCount = 1;

D3D11_INPUT_ELEMENT_DESC Renderer::QuadElements[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
int Renderer::QuadElementCount = 2;

D3D11_INPUT_ELEMENT_DESC Renderer::PacketElements[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXTURE",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXTURE",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
int Renderer::PacketElementCount = 3;

int Renderer::Release(void)
{
	// first unbind everything from the pipeline
	m_pOutputTex->SetResource(NULL);
	m_pWaterPosTex->SetResource(NULL);
	m_pWaterTerrainTex->SetResource(NULL);
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device* pd3dDevice;
	context->GetDevice(&pd3dDevice);
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->Apply(0, context);
	heightTextureRV->Release();
	heightTextureTV->Release();
	posTextureRV->Release();
	posTextureTV->Release();
	aaTextureRV->Release();
	aaTextureTV->Release();
	aaDepthStencilView->Release();
	waterTerrainTextureRV->Release();
	effect->Release();
	heightTex->Release();
	posTex->Release();
	antialiasTex->Release();
	antialiasDepthStencil->Release();
	//	SAFE_RELEASE( pMesh );
	displayMesh->Release();
	displayMeshIdx->Release();
	quadMesh->Release();
	heightFieldMesh->Release();
	packetPointMesh->Release();
	if (boxMesh)
		boxMesh->Release();
	delete[](packetData);
	m_pTerrain.Release();
	pd3dDevice->Release();
	return 0;
}

Renderer::Renderer()
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device* pd3dDevice;
	context->GetDevice(&pd3dDevice);

	WCHAR str[MAX_PATH];
	DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"shaders.cso");
	D3DX11CreateEffectFromFile(str, 0, pd3dDevice, &effect);

	vertexLayout = NULL;
	heightTex = NULL;
	posTex = NULL;
	antialiasTex = NULL;
	antialiasDepthStencil = NULL;

	packetData = new RenderPacketVertex[10000];

	m_pRenderBoxTechnique = effect->GetTechniqueByName("RenderBox");
	m_pRenderSkyTechnique = effect->GetTechniqueByName("RenderSky");

	m_pDisplayMicroMeshTechnique = effect->GetTechniqueByName("DisplayMicroMesh");
	m_pAddPacketsDisplacementTechnique = effect->GetTechniqueByName("AddPacketDisplacement");
	m_pRasterizeWaveMeshPositionTechnique = effect->GetTechniqueByName("RasterizeWaveMeshPosition");

	m_pDisplayTerrain = effect->GetTechniqueByName("DisplayTerrain");
	m_pDisplayAATechnique = effect->GetTechniqueByName("RenderAA");

	m_pOutputTex = effect->GetVariableByName("waterHeightTex")->AsShaderResource();
	m_pWaterPosTex = effect->GetVariableByName("waterSampleTex")->AsShaderResource();
	m_pWaterTerrainTex = effect->GetVariableByName("terrainTex")->AsShaderResource();

	m_pDiffX = effect->GetVariableByName("g_diffX")->AsScalar();
	m_pDiffY = effect->GetVariableByName("g_diffY")->AsScalar();
	g_pmWorldViewProjection = effect->GetVariableByName("mvpMat")->AsMatrix();
	g_pmWorld = effect->GetVariableByName("worldMat")->AsMatrix();

	g_pBoxPos = effect->GetVariableByName("boxPos")->AsScalar();
	g_pTexCube = effect->GetVariableByName("skyTex")->AsShaderResource();
	CreateWICTextureFromFile(pd3dDevice, WATER_TERRAIN_FILE, nullptr, &waterTerrainTextureRV);
	m_pWaterTerrainTex->SetResource(waterTerrainTextureRV);

	VertexPosColor vertices[] =
	{
		//立方体
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.1f, 0.6f, 0.2f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.2f, 0.7f, 0.12f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.1f, 1.0f, 0.08f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.1f, 1.0f, 0.3f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.1f, 0.5f, 0.15f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 0.6f, 0.1f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.2f, 0.5f, 0.4f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.1f, 0.6f, 0.2f, 1.0f) }
	};
	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	pd3dDevice->CreateBuffer(&vbd, &InitData, &boxMesh);
	DWORD indices[] = {
		// 正面
		0, 1, 2,
		2, 3, 0,
		// 左面
		4, 5, 1,
		1, 0, 4,
		// 顶面
		1, 5, 6,
		6, 2, 1,
		// 背面
		7, 6, 5,
		5, 4, 7,
		// 右面
		3, 2, 6,
		6, 7, 3,
		// 底面
		4, 0, 3,
		3, 7, 4
	};
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof indices;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = indices;
	pd3dDevice->CreateBuffer(&ibd, &InitData, &boxIdx);

	QuadVertex pd[6] = {
		{XMFLOAT2(-1.0f,-1.0f),XMFLOAT2(0.0f,1.0f)},
		{XMFLOAT2(1.0f,-1.0f),XMFLOAT2(1.0f,1.0f)},
		{XMFLOAT2(-1.0f,1.0f),XMFLOAT2(0.0f,0.0f)},
		{XMFLOAT2(1.0f,-1.0f),XMFLOAT2(1.0f,1.0f)},
		{XMFLOAT2(1.0f,1.0f),XMFLOAT2(1.0f,0.0f)},
		{XMFLOAT2(-1.0f,1.0f),XMFLOAT2(0.0f,0.0f)},
	};

	D3D11_BUFFER_DESC bd = { 0 };
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(QuadVertex) * 2 * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA srd = { pd, 0, 0 };
	pd3dDevice->CreateBuffer(&bd, &srd, &quadMesh);

	// 生成地形分辨率地图
	int width = 2048;
	int height = 2048;
	SimpleVertex* dpd = new SimpleVertex[width * height * 3 * 2];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
		{
			dpd[(y * width + x) * 2 * 3 + 0].pos = XMFLOAT2(2.0f * ((float)(x) / width - 0.5f), 2.0f * ((float)(y) / height - 0.5f));
			dpd[(y * width + x) * 2 * 3 + 1].pos = XMFLOAT2(2.0f * ((float)(x) / width - 0.5f), 2.0f * ((float)(y + 1) / height - 0.5f));
			dpd[(y * width + x) * 2 * 3 + 2].pos = XMFLOAT2(2.0f * ((float)(x + 1) / width - 0.5f), 2.0f * ((float)(y) / height - 0.5f));
			dpd[(y * width + x) * 2 * 3 + 3].pos = XMFLOAT2(2.0f * ((float)(x) / width - 0.5f), 2.0f * ((float)(y + 1) / height - 0.5f));
			dpd[(y * width + x) * 2 * 3 + 4].pos = XMFLOAT2(2.0f * ((float)(x + 1) / width - 0.5f), 2.0f * ((float)(y + 1) / height - 0.5f));
			dpd[(y * width + x) * 2 * 3 + 5].pos = XMFLOAT2(2.0f * ((float)(x + 1) / width - 0.5f), 2.0f * ((float)(y) / height - 0.5f));
		}
	heightfieldVertNum = width * height * 3 * 2;
	bd.ByteWidth = sizeof(SimpleVertex) * heightfieldVertNum;
	srd = { dpd, 0, 0 };
	pd3dDevice->CreateBuffer(&bd, &srd, &heightFieldMesh);
	delete[](dpd);

	displayMesh = NULL;
	displayMeshIdx = NULL;
	UpdateDisplayMesh();

	bd.ByteWidth = sizeof(RenderPacketVertex) * 10000;
	srd = { packetData, 0, 0 };
	pd3dDevice->CreateBuffer(&bd, &srd, &packetPointMesh);

	pd3dDevice->Release();
}

void Renderer::UpdateDisplayMesh()
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device* pd3dDevice;
	context->GetDevice(&pd3dDevice);

	HRESULT hr;
	int width = (int)(2 * DXUTGetDXGIBackBufferSurfaceDesc()->Width);
	int height = (int)(2 * DXUTGetDXGIBackBufferSurfaceDesc()->Height);

	SimpleVertex* dpd = new SimpleVertex[width * height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			dpd[y * width + x].pos = XMFLOAT2(((float)(x)+0.5f) / width, ((float)(y)+0.5f) / height);
	displayMeshVertNum = (width) * (height);
	D3D11_BUFFER_DESC bd = { 0 };
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * displayMeshVertNum;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.StructureByteStride = sizeof(SimpleVertex);
	D3D11_SUBRESOURCE_DATA srd = { dpd, 0, 0 };
	if (displayMesh != NULL)
		displayMesh->Release();
	pd3dDevice->CreateBuffer(&bd, &srd, &displayMesh);
	delete[](dpd);

	UINT32* idB = new UINT32[3 * 2 * (width - 1) * (height - 1)];
	for (int y = 0; y < height - 1; y++)
		for (int x = 0; x < width - 1; x++)
		{
			idB[3 * 2 * (y * (width - 1) + x) + 0] = y * width + x;
			idB[3 * 2 * (y * (width - 1) + x) + 1] = y * width + x + 1;
			idB[3 * 2 * (y * (width - 1) + x) + 2] = (y + 1) * width + x;
			idB[3 * 2 * (y * (width - 1) + x) + 3] = y * width + x + 1;
			idB[3 * 2 * (y * (width - 1) + x) + 4] = (y + 1) * width + x + 1;
			idB[3 * 2 * (y * (width - 1) + x) + 5] = (y + 1) * width + x;
		}
	displayMeshIndexNum = 3 * 2 * (width - 1) * (height - 1);
	bd = { 0 };
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(UINT32) * displayMeshIndexNum;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.StructureByteStride = sizeof(UINT32);
	srd = { idB, 0, 0 };
	if (displayMeshIdx != NULL)
		displayMeshIdx->Release();
	pd3dDevice->CreateBuffer(&bd, &srd, &displayMeshIdx);
	delete[](idB);

	width = (int)(2 * DXUTGetDXGIBackBufferSurfaceDesc()->Width);
	height = (int)(4 * DXUTGetDXGIBackBufferSurfaceDesc()->Height);
	viewport.Width = (float)(width);
	viewport.Height = (float)(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	m_pDiffX->SetFloat((float)(width));
	m_pDiffY->SetFloat((float)(height));
	DXGI_SAMPLE_DESC samdesc;
	samdesc.Count = 1;
	samdesc.Quality = 0;
	D3D11_TEXTURE2D_DESC texdesc;
	ZeroMemory(&texdesc, sizeof(D3D11_TEXTURE2D_DESC));
	texdesc.MipLevels = 1;
	texdesc.ArraySize = 1;
	texdesc.SampleDesc = samdesc;
	texdesc.Usage = D3D11_USAGE_DEFAULT;
	texdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texdesc.Width = width;
	texdesc.Height = height;
	if (heightTex != NULL)
	{
		heightTextureRV->Release();
		heightTextureTV->Release();
		heightTex->Release();
	}
	hr = pd3dDevice->CreateTexture2D(&texdesc, NULL, &heightTex);
	hr = pd3dDevice->CreateShaderResourceView(heightTex, NULL, &heightTextureRV);
	hr = pd3dDevice->CreateRenderTargetView(heightTex, NULL, &heightTextureTV);
	if (posTex != NULL)
	{
		posTextureRV->Release();
		posTextureTV->Release();
		posTex->Release();
	}
	hr = pd3dDevice->CreateTexture2D(&texdesc, NULL, &posTex);
	hr = pd3dDevice->CreateShaderResourceView(posTex, NULL, &posTextureRV);
	hr = pd3dDevice->CreateRenderTargetView(posTex, NULL, &posTextureTV);
	aaViewport.Width = (float)(4 * DXUTGetDXGIBackBufferSurfaceDesc()->Width);
	aaViewport.Height = (float)(4 * DXUTGetDXGIBackBufferSurfaceDesc()->Height);
	aaViewport.MinDepth = 0.0f;
	aaViewport.MaxDepth = 1.0f;
	aaViewport.TopLeftX = 0;
	aaViewport.TopLeftY = 0;
	texdesc.Width = 4 * DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	texdesc.Height = 4 * DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	if (antialiasTex != NULL)
	{
		antialiasTex->Release();
		aaTextureRV->Release();
		aaTextureTV->Release();
	}
	hr = pd3dDevice->CreateTexture2D(&texdesc, NULL, &antialiasTex);
	hr = pd3dDevice->CreateShaderResourceView(antialiasTex, NULL, &aaTextureRV);
	hr = pd3dDevice->CreateRenderTargetView(antialiasTex, NULL, &aaTextureTV);
	texdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	texdesc.Format = DXGI_FORMAT_D32_FLOAT;
	if (antialiasDepthStencil != NULL)
	{
		aaDepthStencilView->Release();
		antialiasDepthStencil->Release();
	}
	hr = pd3dDevice->CreateTexture2D(&texdesc, NULL, &antialiasDepthStencil);
	hr = pd3dDevice->CreateDepthStencilView(antialiasDepthStencil, NULL, &aaDepthStencilView);
	pd3dDevice->Release();
}

void Renderer::InitiateWavefield(XMMATRIX& mWorld, XMMATRIX& mWorldViewProjectionWide)
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device* pd3dDevice;
	context->GetDevice(&pd3dDevice);
	HRESULT hr;
	ID3D11RenderTargetView* old_pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* old_pDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	context->RSGetViewports(&NumViewports, &pViewports[0]);

	g_pmWorld->SetMatrix((float*)&mWorld);
	g_pmWorldViewProjection->SetMatrix((float*)&mWorldViewProjectionWide);
	context->RSSetViewports(1, &viewport);


	// 清楚目标视图的颜色缓存
	const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(posTextureTV, bgColor);
	context->ClearRenderTargetView(heightTextureTV, bgColor);

	D3DX11_PASS_DESC PassDesc;
	UINT stride, offset;
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	if (FAILED(pd3dDevice->CreateInputLayout(QuadElements, QuadElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &vertexLayout)))
		return;
	context->IASetInputLayout(vertexLayout);
	vertexLayout->Release();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	stride = sizeof(QuadVertex);
	offset = 0;
	context->IASetVertexBuffers(0, 1, &quadMesh, &stride, &offset);
	context->OMSetRenderTargets(1, &posTextureTV, NULL);
	m_pOutputTex->SetResource(heightTextureRV); //将创建的纹理视图赋值给effect中的全局变量
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->Apply(0, context);
	context->Draw(2 * 3, 0);

	ID3D11ShaderResourceView* texture;
	CreateDDSTextureFromFile(pd3dDevice, L"..\\Texture\\grass.dds", nullptr, &texture);
	m_pTerrain.SetBuffer(pd3dDevice, Geometry::CreateTerrain(XMFLOAT2(160.0f, 160.0f),
		XMUINT2(50, 50), XMFLOAT2(100.0f, 100.0f),
		[](float x, float z) { return 0.1f * (z * sinf(0.1f * x) + x * cosf(0.1f * z)); },	// 高度函数
		[](float x, float z) { return XMFLOAT3{ -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z), 1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z) }; }));		// 法向量函数
	m_pTerrain.SetTexture(texture);

	V(m_pOutputTex->SetResource(NULL));
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->Apply(0, context);
	context->OMSetRenderTargets(1, &old_pRTV, old_pDSV);  //restore old render targets
	context->RSSetViewports(NumViewports, &pViewports[0]);
	m_pTerrain.Release();
	pd3dDevice->Release();
}

void Renderer::EvaluatePackets(int usedpackets)
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device* pd3dDevice;
	context->GetDevice(&pd3dDevice);
	HRESULT hr;
	ID3D11RenderTargetView* old_pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* old_pDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	context->RSGetViewports(&NumViewports, &pViewports[0]);
	context->RSSetViewports(1, &viewport);
	D3DX11_PASS_DESC PassDesc;
	UINT stride, offset;
	ID3D11InputLayout* pCurveVertexLayout;
	context->OMSetRenderTargets(1, &heightTextureTV, NULL);
	m_pAddPacketsDisplacementTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	if (FAILED(pd3dDevice->CreateInputLayout(PacketElements, PacketElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pCurveVertexLayout)))
		return;
	context->IASetInputLayout(pCurveVertexLayout);
	pCurveVertexLayout->Release();
	stride = sizeof(RenderPacketVertex);
	offset = 0;
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->IASetVertexBuffers(0, 1, &packetPointMesh, &stride, &offset);
	m_pAddPacketsDisplacementTechnique->GetPassByIndex(0)->Apply(0, context);
	context->Draw(usedpackets, 0);
	V(m_pOutputTex->SetResource(NULL));
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->Apply(0, context);
	context->OMSetRenderTargets(1, &old_pRTV, old_pDSV);
	context->RSSetViewports(NumViewports, &pViewports[0]);
	pd3dDevice->Release();
}

void Renderer::RenderBox()
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device* pd3dDevice;
	context->GetDevice(&pd3dDevice);
	D3DX11_PASS_DESC PassDesc;
	ID3D11InputLayout* pBoxVertexLayout;
	m_pRenderBoxTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	if (FAILED(pd3dDevice->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout), PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pBoxVertexLayout)))
		return;
	context->IASetInputLayout(pBoxVertexLayout);
	pBoxVertexLayout->Release();
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	context->IASetIndexBuffer(boxIdx, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 1, &boxMesh, &stride, &offset);
	context->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0, 0);

	m_pRenderBoxTechnique->GetPassByIndex(0)->Apply(0, context);
	context->DrawIndexed(36, 0, 0);
	pd3dDevice->Release();
}

void Renderer::DisplayScene(bool showBox, int usedpackets, XMMATRIX& mWorldViewProjection)
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device* pd3dDevice;
	context->GetDevice(&pd3dDevice);
	HRESULT hr;

	ID3D11RenderTargetView* old_pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* old_pDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	context->RSGetViewports(&NumViewports, &pViewports[0]);
	context->RSSetViewports(1, &aaViewport);

	const float bgColor[4] = { 0.3f, 0.6f, 0.8f, 0.0f };
	context->ClearRenderTargetView(aaTextureTV, bgColor);
	context->ClearDepthStencilView(aaDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	context->OMSetRenderTargets(1, &aaTextureTV, aaDepthStencilView);

	g_pmWorldViewProjection->SetMatrix((float*)&mWorldViewProjection);

	D3DX11_PASS_DESC PassDesc;
	ID3D11InputLayout* pCurveVertexLayout;
	m_pDisplayMicroMeshTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	if (FAILED(pd3dDevice->CreateInputLayout(SimpleVertexElements, SimpleVertexElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pCurveVertexLayout)))
		return;
	context->IASetInputLayout(pCurveVertexLayout);
	pCurveVertexLayout->Release();
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	V(m_pOutputTex->SetResource(heightTextureRV));
	context->IASetVertexBuffers(0, 1, &heightFieldMesh, &stride, &offset);
	context->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0, 0);

	m_pDisplayTerrain->GetPassByIndex(0)->Apply(0, context);
	context->Draw(heightfieldVertNum, 0);

	V(m_pOutputTex->SetResource(heightTextureRV));
	V(m_pWaterPosTex->SetResource(posTextureRV));
	context->IASetVertexBuffers(0, 1, &displayMesh, &stride, &offset);
	context->IASetIndexBuffer(displayMeshIdx, DXGI_FORMAT_R32_UINT, 0);
	m_pDisplayMicroMeshTechnique->GetPassByIndex(0)->Apply(0, context);
	context->DrawIndexed(displayMeshIndexNum, 0, 0);

	if (showBox)
	{
		RenderBox();
	}

	context->OMSetRenderTargets(1, &old_pRTV, old_pDSV);
	context->RSSetViewports(NumViewports, &pViewports[0]);

	m_pDisplayAATechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	if (FAILED(pd3dDevice->CreateInputLayout(QuadElements, QuadElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &vertexLayout)))
		return;
	context->IASetInputLayout(vertexLayout);
	vertexLayout->Release();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	stride = sizeof(QuadVertex);
	offset = 0;
	context->IASetVertexBuffers(0, 1, &quadMesh, &stride, &offset);
	V(m_pOutputTex->SetResource(aaTextureRV));
	V(m_pDiffX->SetFloat(1.0f / (float)(aaViewport.Width * 4.0f / 4)));
	V(m_pDiffY->SetFloat(1.0f / (float)(aaViewport.Height * 4.0f / 4)));
	m_pDisplayAATechnique->GetPassByIndex(0)->Apply(0, context);
	context->Draw(2 * 3, 0);

	V(m_pDiffX->SetFloat((float)(viewport.Width)));
	V(m_pDiffY->SetFloat((float)(viewport.Height)));
	V(m_pWaterPosTex->SetResource(NULL));
	V(m_pOutputTex->SetResource(NULL));
	m_pDisplayMicroMeshTechnique->GetPassByIndex(0)->Apply(0, context);
	pd3dDevice->Release();
}

