// Include the OS headers
//-----------------------
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
#include "Render.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"

#include <ScreenGrab.h>



D3D11_INPUT_ELEMENT_DESC Render::SimpleVertexElements[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
int Render::SimpleVertexElementCount = 1;


D3D11_INPUT_ELEMENT_DESC Render::QuadElements[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
int Render::QuadElementCount = 2;


D3D11_INPUT_ELEMENT_DESC Render::PacketElements[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXTURE",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
int Render::PacketElementCount = 3;



int Render::Release(void)
{
	// first unbind everything from the pipeline
	m_pOutputTex->SetResource(NULL);
	m_pWaterPosTex->SetResource(NULL);
	m_pWaterTerrainTex->SetResource(NULL);
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device *pd3dDevice;
	context->GetDevice(&pd3dDevice);
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->Apply(0, context);
	m_heightTextureRV->Release();
	m_heightTextureTV->Release();
	m_posTextureRV->Release();
	m_posTextureTV->Release();
	m_AATextureRV->Release();
	m_AATextureTV->Release();
	m_pAADepthStencilView->Release();
	m_waterTerrainTextureRV->Release();
	g_pEffect11->Release();
	m_heightTexture->Release();
	m_posTexture->Release();
	m_AATexture->Release();
	m_pAADepthStencil->Release();
	//	SAFE_RELEASE( pMesh );
	m_pDisplayMesh->Release();
	m_pDisplayMeshIndex->Release();
	m_pQuadMesh->Release();
	m_pHeightfieldMesh->Release();
	m_ppacketPointMesh->Release();
	if(m_pBoxMesh)
		m_pBoxMesh->Release();
	delete[](m_packetData);
	m_pTerrain.Release();
	pd3dDevice->Release();
	return 0;
}



Render::Render()
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device *pd3dDevice;
	context->GetDevice(&pd3dDevice);

	// Read and compile the D3DX effect file
	WCHAR str[MAX_PATH];
	DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"WavePackets.cso");
	D3DX11CreateEffectFromFile(str, 0, pd3dDevice, &g_pEffect11);

	m_pVertexLayout = NULL;
	m_heightTexture = NULL;
	m_posTexture = NULL;
	m_AATexture = NULL;
	m_pAADepthStencil = NULL;
	
	// circular packet initialization
	m_packetData = new PACKET_Vertex[PACKET_GPU_BUFFER_SIZE];

	// connect to shader variables
	m_pRenderBoxTechnique = g_pEffect11->GetTechniqueByName("RenderBox");
	m_pRenderSkyTechnique= g_pEffect11->GetTechniqueByName("RenderSky");

	m_pDisplayMicroMeshTechnique = g_pEffect11->GetTechniqueByName( "DisplayMicroMesh" );
	m_pAddPacketsDisplacementTechnique = g_pEffect11->GetTechniqueByName( "AddPacketDisplacement" );
	m_pRasterizeWaveMeshPositionTechnique = g_pEffect11->GetTechniqueByName( "RasterizeWaveMeshPosition" );

	m_pDisplayTerrain = g_pEffect11->GetTechniqueByName( "DisplayTerrain" );
	m_pDisplayAATechnique = g_pEffect11->GetTechniqueByName( "RenderAA" );

	m_pOutputTex = g_pEffect11->GetVariableByName( "g_waterHeightTex" )->AsShaderResource();
	m_pWaterPosTex = g_pEffect11->GetVariableByName( "g_waterPosTex" )->AsShaderResource();
	m_pWaterTerrainTex = g_pEffect11->GetVariableByName( "g_waterTerrainTex" )->AsShaderResource();

	m_pDiffX = g_pEffect11->GetVariableByName( "g_diffX")->AsScalar();
	m_pDiffY = g_pEffect11->GetVariableByName( "g_diffY")->AsScalar();
	g_pmWorldViewProjection = g_pEffect11->GetVariableByName("g_mWorldViewProjection")->AsMatrix();
	g_pmWorld = g_pEffect11->GetVariableByName("g_mWorld")->AsMatrix();

	g_pBoxPos= g_pEffect11->GetVariableByName("g_BoxPos")->AsScalar();
	g_pTexCube = g_pEffect11->GetVariableByName("g_TexCube")->AsShaderResource();
	// load all textures and connect to shader variables
	CreateWICTextureFromFile(pd3dDevice, WATER_TERRAIN_FILE, nullptr, &m_waterTerrainTextureRV);
	m_pWaterTerrainTex->SetResource( m_waterTerrainTextureRV );

	// InitiateBox
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
	pd3dDevice->CreateBuffer(&vbd, &InitData, &m_pBoxMesh);
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
	pd3dDevice->CreateBuffer(&ibd, &InitData, &m_pBoxIndex);

	// create a screen space quad (for ground plane rendering and for anti aliasing)
	QUAD_Vertex pd[6] = {
		{XMFLOAT2(-1.0f,-1.0f),XMFLOAT2(0.0f,1.0f)},
		{XMFLOAT2(1.0f,-1.0f),XMFLOAT2(1.0f,1.0f)},
		{XMFLOAT2(-1.0f,1.0f),XMFLOAT2(0.0f,0.0f)},
		{XMFLOAT2(1.0f,-1.0f),XMFLOAT2(1.0f,1.0f)},
		{XMFLOAT2(1.0f,1.0f),XMFLOAT2(1.0f,0.0f)},
		{XMFLOAT2(-1.0f,1.0f),XMFLOAT2(0.0f,0.0f)},
	};

	D3D11_BUFFER_DESC bd = { 0 };
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(QUAD_Vertex) * 2 * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	D3D11_SUBRESOURCE_DATA srd = { pd, 0, 0 };
	pd3dDevice->CreateBuffer(&bd, &srd, &m_pQuadMesh);

	// 生成地形分辨率地图
	// generate a heightfield mesh for the boundaries
	int width = 2048;
	int height = 2048;
	SIMPLE_Vertex *dpd = new SIMPLE_Vertex[width*height*3*2];
	for (int y=0; y<height; y++)
		for (int x=0; x<width; x++)
		{
			dpd[(y*width + x) * 2 * 3 + 0].pos = XMFLOAT2(2.0f*((float)(x) / width - 0.5f), 2.0f*((float)(y) / height - 0.5f));
			dpd[(y*width + x) * 2 * 3 + 1].pos = XMFLOAT2(2.0f*((float)(x) / width - 0.5f), 2.0f*((float)(y + 1) / height - 0.5f));
			dpd[(y*width + x) * 2 * 3 + 2].pos = XMFLOAT2(2.0f*((float)(x + 1) / width - 0.5f), 2.0f*((float)(y) / height - 0.5f));
			dpd[(y*width + x) * 2 * 3 + 3].pos = XMFLOAT2(2.0f*((float)(x) / width - 0.5f), 2.0f*((float)(y + 1) / height - 0.5f));
			dpd[(y*width + x) * 2 * 3 + 4].pos = XMFLOAT2(2.0f*((float)(x + 1) / width - 0.5f), 2.0f*((float)(y + 1) / height - 0.5f));
			dpd[(y*width + x) * 2 * 3 + 5].pos = XMFLOAT2(2.0f*((float)(x + 1) / width - 0.5f), 2.0f*((float)(y) / height - 0.5f));
		}
	m_heightfieldVertNum = width*height * 3 * 2;
	bd.ByteWidth = sizeof(SIMPLE_Vertex) * m_heightfieldVertNum;
	srd = { dpd, 0, 0 };
	pd3dDevice->CreateBuffer(&bd, &srd, &m_pHeightfieldMesh);
	delete[](dpd);

	// initiate the display mesh
	m_pDisplayMesh = NULL;
	m_pDisplayMeshIndex = NULL;
	UpdateDisplayMesh();

	// generate a point mesh for all packets, will be updated each frame
	bd.ByteWidth = sizeof(PACKET_Vertex) * PACKET_GPU_BUFFER_SIZE;
	srd = { m_packetData, 0, 0 };
	pd3dDevice->CreateBuffer(&bd, &srd, &m_ppacketPointMesh);

	pd3dDevice->Release();
}





void Render::UpdateDisplayMesh()
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device *pd3dDevice;
	context->GetDevice(&pd3dDevice);

	HRESULT hr;
	int width = (int)(WAVEMESH_WIDTH_FACTOR*DXUTGetDXGIBackBufferSurfaceDesc()->Width);
	int height = (int)(WAVEMESH_HEIGHT_FACTOR*DXUTGetDXGIBackBufferSurfaceDesc()->Height);

	SIMPLE_Vertex *dpd = new SIMPLE_Vertex[width*height];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			dpd[y*width + x].pos = XMFLOAT2(((float)(x)+0.5f) / width, ((float)(y)+0.5f) / height);
	m_displayMeshVertNum = (width)*(height);
	D3D11_BUFFER_DESC bd = { 0 };
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SIMPLE_Vertex) * m_displayMeshVertNum;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.StructureByteStride = sizeof(SIMPLE_Vertex);
	D3D11_SUBRESOURCE_DATA srd = { dpd, 0, 0 };
	if (m_pDisplayMesh != NULL)
		m_pDisplayMesh->Release();
	pd3dDevice->CreateBuffer(&bd, &srd, &m_pDisplayMesh);
	delete[](dpd);

	// create the index buffer 
	UINT32 *idB = new UINT32[3 * 2 * (width - 1)*(height - 1)];
	for (int y = 0; y < height - 1; y++)
		for (int x = 0; x < width - 1; x++)
		{
			idB[3 * 2 * (y*(width - 1) + x) + 0] = y*width + x;
			idB[3 * 2 * (y*(width - 1) + x) + 1] = y*width + x + 1;
			idB[3 * 2 * (y*(width - 1) + x) + 2] = (y + 1)*width + x;
			idB[3 * 2 * (y*(width - 1) + x) + 3] = y*width + x + 1;
			idB[3 * 2 * (y*(width - 1) + x) + 4] = (y + 1)*width + x + 1;
			idB[3 * 2 * (y*(width - 1) + x) + 5] = (y + 1)*width + x;
		}
	m_displayMeshIndexNum = 3 * 2 * (width - 1)*(height - 1);
	bd = { 0 };
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(UINT32) * m_displayMeshIndexNum;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.StructureByteStride = sizeof(UINT32);
	srd = { idB, 0, 0 };
	if (m_pDisplayMeshIndex != NULL)
		m_pDisplayMeshIndex->Release();
	pd3dDevice->CreateBuffer(&bd, &srd, &m_pDisplayMeshIndex);
	delete[](idB);

	//create the GPU textures, viewports etc.
	width = (int)(WAVETEX_WIDTH_FACTOR*DXUTGetDXGIBackBufferSurfaceDesc()->Width);
	height = (int)(WAVETEX_HEIGHT_FACTOR*DXUTGetDXGIBackBufferSurfaceDesc()->Height);
	m_vp.Width = (float)(width);
	m_vp.Height = (float)(height);
	m_vp.MinDepth = 0.0f;
	m_vp.MaxDepth = 1.0f;
	m_vp.TopLeftX = 0;
	m_vp.TopLeftY = 0;
	m_pDiffX->SetFloat((float)(width));
	m_pDiffY->SetFloat((float)(height));
	DXGI_SAMPLE_DESC samdesc;
	samdesc.Count = 1;
	samdesc.Quality = 0;
	// texture to store the wave field position and displacement data
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
	if (m_heightTexture != NULL)
	{
		m_heightTextureRV->Release();
		m_heightTextureTV->Release();
		m_heightTexture->Release();
	}
	hr = pd3dDevice->CreateTexture2D( &texdesc, NULL, &m_heightTexture);
	hr = pd3dDevice->CreateShaderResourceView( m_heightTexture, NULL, &m_heightTextureRV );
	hr = pd3dDevice->CreateRenderTargetView(   m_heightTexture, NULL, &m_heightTextureTV );
	if (m_posTexture != NULL)
	{
		m_posTextureRV->Release();
		m_posTextureTV->Release();
		m_posTexture->Release();
	}
	hr = pd3dDevice->CreateTexture2D(&texdesc, NULL, &m_posTexture);
	hr = pd3dDevice->CreateShaderResourceView( m_posTexture, NULL, &m_posTextureRV );
	hr = pd3dDevice->CreateRenderTargetView(   m_posTexture, NULL, &m_posTextureTV );
	// large anti aliasing texture
	m_vpAA.Width = (float)(AA_OVERSAMPLE_FACTOR*DXUTGetDXGIBackBufferSurfaceDesc()->Width);
	m_vpAA.Height = (float)(AA_OVERSAMPLE_FACTOR*DXUTGetDXGIBackBufferSurfaceDesc()->Height);
	m_vpAA.MinDepth = 0.0f;
	m_vpAA.MaxDepth = 1.0f;
	m_vpAA.TopLeftX = 0;
	m_vpAA.TopLeftY = 0;
	texdesc.Width = AA_OVERSAMPLE_FACTOR*DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	texdesc.Height = AA_OVERSAMPLE_FACTOR*DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	if (m_AATexture != NULL)
	{
		m_AATexture->Release();
		m_AATextureRV->Release();
		m_AATextureTV->Release();
	}
	hr = pd3dDevice->CreateTexture2D( &texdesc, NULL, &m_AATexture);
	hr = pd3dDevice->CreateShaderResourceView( m_AATexture, NULL, &m_AATextureRV );
	hr = pd3dDevice->CreateRenderTargetView(   m_AATexture, NULL, &m_AATextureTV );
	texdesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	texdesc.Format = DXGI_FORMAT_D32_FLOAT;
	if (m_pAADepthStencil != NULL)
	{
		m_pAADepthStencilView->Release();
		m_pAADepthStencil->Release();
	}
	hr = pd3dDevice->CreateTexture2D( &texdesc, NULL, &m_pAADepthStencil);
	hr = pd3dDevice->CreateDepthStencilView( m_pAADepthStencil, NULL, &m_pAADepthStencilView );
	pd3dDevice->Release();
}



void Render::InitiateWavefield(XMMATRIX &mWorld, XMMATRIX &mWorldViewProjectionWide)
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device *pd3dDevice;
	context->GetDevice(&pd3dDevice);
	HRESULT hr;
	//store the old render targets and viewports
	ID3D11RenderTargetView* old_pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* old_pDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	context->RSGetViewports(&NumViewports, &pViewports[0]);

	g_pmWorld->SetMatrix((float*)&mWorld);
	g_pmWorldViewProjection->SetMatrix((float*)&mWorldViewProjectionWide);
	context->RSSetViewports(1, &m_vp);


	// 清楚目标视图的颜色缓存
	const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(m_posTextureTV, bgColor);
	context->ClearRenderTargetView(m_heightTextureTV, bgColor);  // initiate displacement texture with 0 

	D3DX11_PASS_DESC PassDesc;
	UINT stride, offset;
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	if (FAILED(pd3dDevice->CreateInputLayout(QuadElements, QuadElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_pVertexLayout)))
		return;
	context->IASetInputLayout(m_pVertexLayout);
	m_pVertexLayout->Release();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	stride = sizeof(QUAD_Vertex);
	offset = 0;
	// rasterize the current positions of the water waves into the position texture
	context->IASetVertexBuffers(0, 1, &m_pQuadMesh, &stride, &offset);
	context->OMSetRenderTargets(1, &m_posTextureTV, NULL);
	m_pOutputTex->SetResource(m_heightTextureRV); //将创建的纹理视图赋值给effect中的全局变量
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->Apply(0, context);
	context->Draw(2* 3, 0);


	//InitiateTerrain
	ID3D11ShaderResourceView* texture;
	CreateDDSTextureFromFile(pd3dDevice, L"..\\Texture\\grass.dds", nullptr, &texture);
	m_pTerrain.SetBuffer(pd3dDevice, Geometry::CreateTerrain(XMFLOAT2(160.0f, 160.0f),
		XMUINT2(50, 50), XMFLOAT2(100.0f, 100.0f),
		[](float x, float z) { return 0.1f * (z * sinf(0.1f * x) + x * cosf(0.1f * z)); },	// 高度函数
		[](float x, float z) { return XMFLOAT3{ -0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z), 1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z) }; }));		// 法向量函数
	m_pTerrain.SetTexture(texture);



	// sum up: unbind all used textures (this kills annoying debug messages) and restore render targets+viewports
	V(m_pOutputTex->SetResource(NULL));
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->Apply(0, context);
	context->OMSetRenderTargets(1, &old_pRTV, old_pDSV);  //restore old render targets
	context->RSSetViewports(NumViewports, &pViewports[0]);
	m_pTerrain.Release();
	pd3dDevice->Release();
}

void Render::InitiateTerrain(XMMATRIX& mWorld, XMMATRIX& mWorldViewProjectionWide)
{

}


// rasterize wave packet displacements
void Render::EvaluatePackets(int usedpackets)
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device *pd3dDevice;
	context->GetDevice(&pd3dDevice);
	HRESULT hr;
	//store the old render targets and viewports
	ID3D11RenderTargetView* old_pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* old_pDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	context->RSGetViewports(&NumViewports, &pViewports[0]);
	context->RSSetViewports(1, &m_vp);
	D3DX11_PASS_DESC PassDesc;
	UINT stride, offset;
	ID3D11InputLayout* pCurveVertexLayout;
	context->OMSetRenderTargets(1, &m_heightTextureTV, NULL);
	m_pAddPacketsDisplacementTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	if (FAILED(pd3dDevice->CreateInputLayout(PacketElements, PacketElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pCurveVertexLayout)))
		return;
	context->IASetInputLayout(pCurveVertexLayout);
	pCurveVertexLayout->Release();
	stride = sizeof(PACKET_Vertex);
	offset = 0;
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->IASetVertexBuffers(0, 1, &m_ppacketPointMesh, &stride, &offset);
	m_pAddPacketsDisplacementTechnique->GetPassByIndex(0)->Apply(0, context);  // draw the circular packets in this first pass
	context->Draw(usedpackets, 0);
	// sum up: unbind all used textures (kills nasty debug messages) and restore render targets+viewports
	V(m_pOutputTex->SetResource(NULL));
	m_pRasterizeWaveMeshPositionTechnique->GetPassByIndex(0)->Apply(0, context);
	context->OMSetRenderTargets(1, &old_pRTV, old_pDSV);  //restore old render targets
	context->RSSetViewports(NumViewports, &pViewports[0]);
	pd3dDevice->Release();
}

void Render::RenderBox()
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device* pd3dDevice;
	context->GetDevice(&pd3dDevice);
	HRESULT hr;
	// drawBox
	// ******************
	// 给渲染管线各个阶段绑定好所需资源
	//
	D3DX11_PASS_DESC PassDesc;
	ID3D11InputLayout* pBoxVertexLayout;
	m_pRenderBoxTechnique->GetPassByIndex(0)->GetDesc(&PassDesc);
	if (FAILED(pd3dDevice->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout), PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pBoxVertexLayout)))
		return;
	context->IASetInputLayout(pBoxVertexLayout);
	pBoxVertexLayout->Release();
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	context->IASetIndexBuffer(m_pBoxIndex, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 1, &m_pBoxMesh, &stride, &offset);
	context->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0, 0);

	// draw the terrain/boundary heightfield
	m_pRenderBoxTechnique->GetPassByIndex(0)->Apply(0, context);
	context->DrawIndexed(36, 0, 0);
}

// this renders the final image to the screen
void Render::DisplayScene (bool showBox, int usedpackets, XMMATRIX& mWorldViewProjection)
{
	auto context = DXUTGetD3D11DeviceContext();
	ID3D11Device *pd3dDevice;
	context->GetDevice(&pd3dDevice);
	HRESULT hr;

	//first set the AA texture as new render target
	//store the old render targets and viewports
    ID3D11RenderTargetView* old_pRTV = DXUTGetD3D11RenderTargetView();
    ID3D11DepthStencilView* old_pDSV = DXUTGetD3D11DepthStencilView();
	UINT NumViewports = 1;
	D3D11_VIEWPORT pViewports[100];
	context->RSGetViewports( &NumViewports, &pViewports[0]);
	context->RSSetViewports( 1, &m_vpAA );

	// viewport bgColor
	const float bgColor[4] = { 0.3f, 0.6f, 0.8f, 0.0f };
	context->ClearRenderTargetView(m_AATextureTV, bgColor);
	context->ClearDepthStencilView( m_pAADepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );
	context->OMSetRenderTargets( 1, &m_AATextureTV, m_pAADepthStencilView );

	// set the regular (non-wide) view matrix
	g_pmWorldViewProjection->SetMatrix((float*)&mWorldViewProjection);

	// Create the input layout
    D3DX11_PASS_DESC PassDesc;
	ID3D11InputLayout* pCurveVertexLayout;
	m_pDisplayMicroMeshTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	if( FAILED( pd3dDevice->CreateInputLayout( SimpleVertexElements, SimpleVertexElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pCurveVertexLayout ) ) )
		return;
	context->IASetInputLayout( pCurveVertexLayout );
	pCurveVertexLayout->Release();
	UINT stride = sizeof( SIMPLE_Vertex );
	UINT offset = 0;
	context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	V( m_pOutputTex->SetResource( m_heightTextureRV ) );
	context->IASetVertexBuffers(0, 1, &m_pHeightfieldMesh, &stride, &offset);
	context->ClearDepthStencilView( DXUTGetD3D11DepthStencilView(), D3D11_CLEAR_DEPTH, 1.0, 0 );

	// draw the terrain/boundary heightfield
	m_pDisplayTerrain->GetPassByIndex( 0 )->Apply(0, context);
	context->Draw( m_heightfieldVertNum, 0 );

	// draw the water surface mesh using the computed height and position textures
	V( m_pOutputTex->SetResource( m_heightTextureRV ) );
	V( m_pWaterPosTex->SetResource( m_posTextureRV ) );
	context->IASetVertexBuffers(0, 1, &m_pDisplayMesh, &stride, &offset);
	context->IASetIndexBuffer(m_pDisplayMeshIndex, DXGI_FORMAT_R32_UINT, 0);
	m_pDisplayMicroMeshTechnique->GetPassByIndex( 0 )->Apply(0, context);
	context->DrawIndexed(m_displayMeshIndexNum, 0, 0);

	if (showBox)
	{
		RenderBox();
	}

	//FROM HERE DRAWING TO SCREEN
	context->OMSetRenderTargets( 1,  &old_pRTV,  old_pDSV );
	context->RSSetViewports( NumViewports, &pViewports[0]);

	// Create the input layout
    m_pDisplayAATechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
    if( FAILED( pd3dDevice->CreateInputLayout( QuadElements, QuadElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_pVertexLayout ) ) )
        return;
	context->IASetInputLayout( m_pVertexLayout );
	m_pVertexLayout->Release();
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    stride = sizeof( QUAD_Vertex );
    offset = 0;
	context->IASetVertexBuffers( 0, 1, &m_pQuadMesh, &stride, &offset );
	V( m_pOutputTex->SetResource( m_AATextureRV ) );
	V( m_pDiffX->SetFloat( 1.0f/(float)(m_vpAA.Width*4.0f/AA_OVERSAMPLE_FACTOR) ) );
	V( m_pDiffY->SetFloat( 1.0f/(float)(m_vpAA.Height*4.0f/AA_OVERSAMPLE_FACTOR) ) );
	m_pDisplayAATechnique->GetPassByIndex(0)->Apply(0, context);   // downsample the rasterized image to screen
	context->Draw( 2*3, 0 );

	// sum up
	V( m_pDiffX->SetFloat( (float)(m_vp.Width) ) );
	V( m_pDiffY->SetFloat( (float)(m_vp.Height) ) );
	V( m_pWaterPosTex->SetResource( NULL ) );
	V( m_pOutputTex->SetResource( NULL ) );
	m_pDisplayMicroMeshTechnique->GetPassByIndex(0)->Apply(0, context);
	pd3dDevice->Release();
}

