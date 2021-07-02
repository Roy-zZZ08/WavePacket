#pragma warning( disable : 4100 )

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTCamera.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "resource.h"
#include "Packets.h"
#include "Renderer.h"
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace DirectX;

CFirstPersonCamera mainCam;
CDXUTDialogResourceManager dialogResourceManager;
CD3DSettingsDlg settingsDlg;
CDXUTDialog	hud;
CDXUTDialog	buttons;

float waveSpeed = 0.02f;
float InitBoxHeight = 10.0f;
bool showBox = false;
bool waveStart = false;

Packets* g_packets;
Renderer* renderer;
XMVECTOR cursorPos;
int	m_displayedPackets = 0;
int	FrameCount = 0;

#define IDC_RESET   			30
#define IDC_NEWWAVE   			31

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
	*pbNoFurtherProcessing = dialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	if (settingsDlg.IsActive())
	{
		settingsDlg.MsgProc(hWnd, uMsg, wParam, lParam);
		return 0;
	}

	*pbNoFurtherProcessing = hud.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;
	*pbNoFurtherProcessing = buttons.MsgProc(hWnd, uMsg, wParam, lParam);
	if (*pbNoFurtherProcessing)
		return 0;

	mainCam.HandleMessages(hWnd, uMsg, wParam, lParam);

	return 0;
}
void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
}
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	switch (nControlID)
	{
	case IDC_NEWWAVE: {
		showBox = true;
		break;
	}
	case IDC_RESET: {
		g_packets->Reset();
		renderer->g_pBoxPos->SetFloat(InitBoxHeight);
		showBox = false;
		waveStart = false;
		FrameCount = 0;
		break;
	}
	}
}
void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	mainCam.FrameMove(fElapsedTime);
}
bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
	return true;
}

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo* AdapterInfo, UINT Output,
	const CD3D11EnumDeviceInfo* DeviceInfo,
	DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext) {
	return true;
}
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	HRESULT hr;
	auto pd3dImmediateContext = DXUTGetD3D11DeviceContext();

	V_RETURN(dialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
	V_RETURN(settingsDlg.OnD3D11CreateDevice(pd3dDevice));

	// 视角设置
	static const XMVECTORF32 vecPos = { 0.17f, 25.0f, 15.49f, 0.0f };
	static const XMVECTORF32 lookAt = { 5.0f, 0.0f, -5.0f, 0.0f };
	mainCam.SetViewParams(vecPos, lookAt);

	renderer = new Renderer();
	g_packets = new Packets(10000);

	return S_OK;
}
HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
	const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
	HRESULT hr;
	V_RETURN(dialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	V_RETURN(settingsDlg.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));
	float fAspectRatio = pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
	mainCam.SetProjParams(XM_PI / 4.0, fAspectRatio, 0.5f, 4000.0f);
	mainCam.SetEnablePositionMovement(true);
	mainCam.SetResetCursorAfterMove(false);
	mainCam.SetScalers(0.003, 30);
	hud.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
	hud.SetSize(170, 170);
	buttons.SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300);
	buttons.SetSize(170, 300);

	renderer->UpdateDisplayMesh();

	return S_OK;
}

void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
	dialogResourceManager.OnD3D11ReleasingSwapChain();
}

void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	delete(g_packets);
	renderer->Release();
	delete(renderer);

	dialogResourceManager.OnD3D11DestroyDevice();
	settingsDlg.OnD3D11DestroyDevice();
	DXUTGetGlobalResourceCache().OnDestroyDevice();
}
void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
	float fElapsedTime, void* pUserContext)
{
	if (showBox) {
		FrameCount++;
		renderer->yPos = InitBoxHeight - 0.0018 * FrameCount * FrameCount;
		if (renderer->yPos < 0.3f)
		{
			renderer->yPos = 0.3f;
			renderer->yPos += 0.3 * sin(0.1 * FrameCount);
			if (FrameCount > 500)
				showBox = false;
		}
		renderer->g_pBoxPos->SetFloat(renderer->yPos);

		if (FrameCount > 70)
		{
			if (!waveStart) {
				g_packets->CreateCircularWavefront(0.0f, 0.0f, 2.0, 0.2, 3.0, 10000);	// "all"-wavelength impact
				waveStart = true;
			}
		}
	}
	else renderer->g_pBoxPos->SetFloat(InitBoxHeight);

	// 逐帧计算波的更新
	g_packets->AdvectWavePackets(waveSpeed);

	XMMATRIX mWorld = mainCam.GetWorldMatrix();
	XMMATRIX mView = mainCam.GetViewMatrix();
	XMMATRIX mProj = mainCam.GetProjMatrix();
	XMMATRIX mWorldViewProjection = XMMatrixMultiply(mView, mProj);
	CFirstPersonCamera   camWide = mainCam;
	float fAspectRatio = DXUTGetDXGIBackBufferSurfaceDesc()->Width / (FLOAT)DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	camWide.SetProjParams(XM_PI / 3.0, fAspectRatio, 0.1f, 15000.0f);
	XMMATRIX  mWorldViewProjectionWide = XMMatrixMultiply(mView, camWide.GetProjMatrix());

	// 渲染地形
	renderer->InitiateWavefield(mWorld, mWorldViewProjectionWide);

	m_displayedPackets = 0;
	int packetChunk = 0;

	g_packets->packets.for_each_seq([&](auto& packet) { // regular wave packets
		if (!packet.use3rd)
		{
			// 数据写入GPU顶点结构
			renderer->packetData[packetChunk].posDir = XMFLOAT4(packet.midPos.x(), packet.midPos.y(), packet.travelDir.x(), packet.travelDir.y());
			renderer->packetData[packetChunk].att = XMFLOAT4(packet.ampOld, (float)(2.0 * XM_PI / packet.k), (float)(packet.phase), (float)(packet.envelope));
			renderer->packetData[packetChunk].att2 = XMFLOAT4(packet.bending, 0.0f, 0.0f, 0.0f);
			m_displayedPackets++;
			packetChunk++;
			if (packetChunk >= 10000)
			{
				// 渲染Water Packet
				pd3dImmediateContext->UpdateSubresource(renderer->packetPointMesh, 0, NULL, renderer->packetData, 0, 0);
				renderer->EvaluatePackets(packetChunk);
				packetChunk = 0;
			}
		}
	});

	g_packets->ghosts.for_each_seq([&](auto& ghost) {  // ghost packets
		renderer->packetData[packetChunk].att = XMFLOAT4((float)(ghost.ampOld), (float)(2.0 * XM_PI / ghost.k), (float)(ghost.phase), (float)(ghost.envelope));
		renderer->packetData[packetChunk].posDir = XMFLOAT4((float)(ghost.pos.x()), (float)(ghost.pos.y()), (float)(ghost.dir.x()), (float)(ghost.dir.y()));
		renderer->packetData[packetChunk].att2 = XMFLOAT4(ghost.bending, 0.0f, 0.0f, 0.0f);
		m_displayedPackets++;
		packetChunk++;
		if (packetChunk >= 10000)
		{
			pd3dImmediateContext->UpdateSubresource(renderer->packetPointMesh, 0, NULL, renderer->packetData, 0, 0);
			renderer->EvaluatePackets(packetChunk);
			packetChunk = 0;
		}
	});

	pd3dImmediateContext->UpdateSubresource(renderer->packetPointMesh, 0, NULL, renderer->packetData, 0, 0);  // send the wave packet data to the GPU
	renderer->EvaluatePackets(packetChunk);
	renderer->DisplayScene(showBox, min(m_displayedPackets, 10000), mWorldViewProjection);

	XMVECTOR cPos = mainCam.GetEyePt();
	XMVECTOR lPos = mainCam.GetLookAtPt();
	XMVECTOR viewVec = lPos - cPos;
	cursorPos = XMVECTORF32{ 0, 0 };
	if ((XMVectorGetY(viewVec) < 0.0) && (XMVectorGetY(cPos) > 0.0))
		cursorPos = XMVectorSet(XMVectorGetX(cPos) - XMVectorGetX(viewVec) * XMVectorGetY(cPos) / XMVectorGetY(viewVec), XMVectorGetZ(cPos) - XMVectorGetZ(viewVec) * XMVectorGetY(cPos) / XMVectorGetY(viewVec), 0, 0);

	hud.OnRender(fElapsedTime);
	buttons.OnRender(fElapsedTime);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	DXUTSetCallbackMsgProc(MsgProc);
	DXUTSetCallbackKeyboard(OnKeyboard);
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
	DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
	DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
	settingsDlg.Init(&dialogResourceManager);
	hud.Init(&dialogResourceManager);
	buttons.Init(&dialogResourceManager);
	hud.SetCallback(OnGUIEvent);
	buttons.SetCallback(OnGUIEvent);
	buttons.AddButton(IDC_RESET, L"Reset", -400, 100, 200, 40, 'N');
	buttons.AddButton(IDC_NEWWAVE, L"New wave", -400, 150, 200, 40, 'R');
	DXUTInit(true, true, nullptr);
	DXUTSetCursorSettings(true, true);
	DXUTCreateWindow(L"WavePackets Demo");
	DXUTCreateDevice(D3D_FEATURE_LEVEL_10_0, true, 1280, 720);
	DXUTMainLoop();
	return DXUTGetExitCode();
}

