#pragma once

#include <iostream>
#include "SDKmisc.h"
#include "d3dx11effect.h"
#include "DXTrace.h"
#include "GameObject.h"
#include "d3dUtil.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "winmm.lib")

using namespace std;
using namespace DirectX;

struct SimpleVertex
{
	XMFLOAT2 pos;
};

struct QuadVertex
{
	XMFLOAT2 pos;
	XMFLOAT2 tex;
};

struct RenderPacketVertex
{
	XMFLOAT4 posDir;
	XMFLOAT4 att;
	XMFLOAT4 att2;
};

__declspec(align(16)) class Renderer
{
public:
	GameObject m_pTerrain;
	ID3D11Buffer* quadMesh;
	ID3D11Buffer* displayMesh;
	ID3D11Buffer* displayMeshIdx;
	ID3D11Buffer* heightFieldMesh;
	ID3D11Buffer* packetPointMesh;
	ID3D11Buffer* boxMesh;
	ID3D11Buffer* boxIdx;
	ID3D11VertexShader* vertexShader;	    // 顶点着色器
	ID3D11PixelShader* pixelShader;		// 像素着色器
	ID3D11Texture2D* heightTex;
	ID3D11Texture2D* posTex;
	ID3D11Texture2D* antialiasTex;
	ID3D11Texture2D* antialiasDepthStencil;
	ID3D11InputLayout* vertexLayout;
	ID3DX11Effect* effect;
	ID3D11ShaderResourceView* heightTextureRV;
	ID3D11RenderTargetView* heightTextureTV;
	ID3D11ShaderResourceView* posTextureRV;
	ID3D11RenderTargetView* posTextureTV;
	ID3D11ShaderResourceView* aaTextureRV;
	ID3D11RenderTargetView* aaTextureTV;
	ID3D11DepthStencilView* aaDepthStencilView;
	ID3D11ShaderResourceView* waterTerrainTextureRV;
	ID3D11ShaderResourceView* skyTex; //天空盒纹理
	ID3DX11EffectTechnique* m_pRenderBoxTechnique;
	ID3DX11EffectTechnique* m_pRenderSkyTechnique;
	ID3DX11EffectTechnique* m_pAddPacketsDisplacementTechnique;
	ID3DX11EffectTechnique* m_pRasterizeWaveMeshPositionTechnique;
	ID3DX11EffectTechnique* m_pDisplayMicroMeshTechnique;
	ID3DX11EffectTechnique* m_pDisplayTerrain;
	ID3DX11EffectTechnique* m_pDisplayAATechnique;
	ID3DX11EffectShaderResourceVariable* m_pOutputTex;
	ID3DX11EffectShaderResourceVariable* m_pWaterPosTex;
	ID3DX11EffectShaderResourceVariable* m_pWaterTerrainTex;
	ID3DX11EffectShaderResourceVariable* g_pTexCube = nullptr;
	ID3DX11EffectScalarVariable* m_pDiffX;
	ID3DX11EffectScalarVariable* m_pDiffY;
	ID3DX11EffectMatrixVariable* g_pmWorldViewProjection = nullptr;
	ID3DX11EffectMatrixVariable* g_pmWorld = nullptr;
	ID3DX11EffectScalarVariable* g_pBoxPos = nullptr;
	float yPos = 2.0f;
	D3D11_VIEWPORT viewport, aaViewport;
	RenderPacketVertex* packetData;
	int	heightfieldVertNum;
	int	displayMeshVertNum;
	int	displayMeshIndexNum;
	static D3D11_INPUT_ELEMENT_DESC SimpleVertexElements[];
	static D3D11_INPUT_ELEMENT_DESC QuadElements[];
	static D3D11_INPUT_ELEMENT_DESC PacketElements[];
	static int SimpleVertexElementCount;
	static int QuadElementCount;
	static int PacketElementCount;

	Renderer();
	virtual ~Renderer()
	{
	}
	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}
	void operator delete(void* p)
	{
		_mm_free(p);
	}

	int Release();
	void UpdateDisplayMesh();
	void InitiateWavefield(XMMATRIX& mWorld, XMMATRIX& mWorldViewProjectionWide);
	void RenderBox();
	void EvaluatePackets(int usedpackets);
	void DisplayScene(bool showBox, int usedpackets, XMMATRIX& mWorldViewProjection);
};
