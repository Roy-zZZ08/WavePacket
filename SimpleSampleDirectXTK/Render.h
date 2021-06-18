#pragma once

#include <iostream>
#include "GlobalDefs.h"
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


struct SIMPLE_Vertex
{
	XMFLOAT2 pos; // Position
};

struct QUAD_Vertex
{
	XMFLOAT2 pos; // Position
	XMFLOAT2 tex; // Texture coords
};

struct PACKET_Vertex
{
	XMFLOAT4 posDir;	// Position and direction of this wave packet
	XMFLOAT4 att;		// wave packet attributes needed for rendering: x=amplitude, y=wavelength, z=phase offset within packed, w=envelope size
	XMFLOAT4 att2;		// wave packet attributes needed for rendering: x = center of wave bending circle
};


__declspec(align(16)) class Render
{
public:

	GameObject m_pTerrain;

	ID3D11Buffer	*m_pQuadMesh;
	ID3D11Buffer	*m_pDisplayMesh;		// fine mesh that gets projected on the ocean surface
	ID3D11Buffer	*m_pDisplayMeshIndex;
	ID3D11Buffer	*m_pHeightfieldMesh;
	ID3D11Buffer	*m_ppacketPointMesh;

	ID3D11Buffer	*m_pBoxMesh;
	ID3D11Buffer	*m_pBoxIndex;

	ID3D11VertexShader* m_pVertexShader;	    // 顶点着色器
	ID3D11PixelShader* m_pPixelShader;		// 像素着色器

	ID3D11Texture2D	*m_heightTexture;
	ID3D11Texture2D	*m_posTexture; 
	ID3D11Texture2D	*m_AATexture; 
	ID3D11Texture2D	*m_pAADepthStencil; 

	ID3D11InputLayout* m_pVertexLayout;

	ID3DX11Effect* g_pEffect11;

	ID3D11ShaderResourceView	*m_heightTextureRV;
	ID3D11RenderTargetView		*m_heightTextureTV;
	ID3D11ShaderResourceView	*m_posTextureRV;
	ID3D11RenderTargetView		*m_posTextureTV;
	ID3D11ShaderResourceView	*m_AATextureRV;
	ID3D11RenderTargetView		*m_AATextureTV;
	ID3D11DepthStencilView		*m_pAADepthStencilView;
	ID3D11ShaderResourceView	*m_waterTerrainTextureRV;

	ID3D11ShaderResourceView* m_pTextureCube; //天空盒纹理

	ID3DX11EffectTechnique* m_pRenderBoxTechnique;
	ID3DX11EffectTechnique* m_pRenderSkyTechnique;

	ID3DX11EffectTechnique *m_pAddPacketsDisplacementTechnique;
	ID3DX11EffectTechnique *m_pRasterizeWaveMeshPositionTechnique;

	ID3DX11EffectTechnique *m_pDisplayMicroMeshTechnique;
	ID3DX11EffectTechnique *m_pDisplayTerrain;
	ID3DX11EffectTechnique *m_pDisplayAATechnique;

	ID3DX11EffectShaderResourceVariable* m_pOutputTex;
	ID3DX11EffectShaderResourceVariable* m_pWaterPosTex;
	ID3DX11EffectShaderResourceVariable* m_pWaterTerrainTex;
	ID3DX11EffectShaderResourceVariable* g_pTexCube = nullptr;

	ID3DX11EffectScalarVariable*	m_pDiffX;
	ID3DX11EffectScalarVariable*	m_pDiffY;

	ID3DX11EffectMatrixVariable*	g_pmWorldViewProjection = nullptr;
	ID3DX11EffectMatrixVariable*	g_pmWorld = nullptr;
	ID3DX11EffectScalarVariable*	g_pBoxPos = nullptr;
	

	float  yPos=2.0f;

	// viewports for rendering 
	D3D11_VIEWPORT m_vp,m_vpAA;

	// GPU packets 
	PACKET_Vertex *m_packetData;

	// terrain and objects
	int			m_heightfieldVertNum;
	int			m_displayMeshVertNum;
	int			m_displayMeshIndexNum;

	static D3D11_INPUT_ELEMENT_DESC SimpleVertexElements[];
	static D3D11_INPUT_ELEMENT_DESC QuadElements[];
	static D3D11_INPUT_ELEMENT_DESC PacketElements[];
	static int SimpleVertexElementCount;
	static int QuadElementCount;
	static int PacketElementCount;

	virtual ~Render()
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

	Render();
	int Release();
	void UpdateDisplayMesh();
	void InitiateWavefield(XMMATRIX &mWorld, XMMATRIX &mWorldViewProjectionWide);
	//渲染地面
	void RenderBox();
	void InitiateTerrain(XMMATRIX& mWorld, XMMATRIX& mWorldViewProjectionWide);
	void EvaluatePackets(int usedpackets);
	void DisplayScene(bool showBox, int usedpackets, XMMATRIX &mWorldViewProjection);
};
