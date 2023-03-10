//=============================================================================
//
// rendering.cpp
// Author : Ricci Alex
//
//=============================================================================

//=============================================================================
// インクルードファイル
//=============================================================================
#include "rendering.h"
#include "object.h"
#include "camera.h"
#include "application.h"
#include <stdio.h>
#include "debugProc.h"
#include "membraneLighting.h"
#include "phongShading.h"
//#include "fade.h"

//=============================================================================
//								静的変数の初期化
//=============================================================================
const float CRenderer::DEFAULT_FOG_NEAR = 1500.0f;				//フォグの開始距離
const float CRenderer::DEFAULT_FOG_FAR = 3600.0f;				//フォグの終了距離
const float CRenderer::DEEP_FOG_NEAR = 300.0f;					//フォグの開始距離(密度高い)
const float CRenderer::DEEP_FOG_FAR = 600.0f;					//フォグの終了距離(密度高い)

//=============================================================================
// コンストラクタ
//=============================================================================
CRenderer::CRenderer()
{
	//メンバー変数をクリアする
	m_pD3D = nullptr;						
	m_pD3DDevice = nullptr;		
	m_pFont = nullptr;		

	m_nCntFog = 0;
	m_nCntColorFog = 0;
	m_fFrameFog = Vec2Null;
	m_targetCol = ColorNull;
	m_fogColor = ColorNull;
	m_colorSpeed = ColorNull;
	m_fFogNear = 0.0f;
	m_fFogFar = 0.0f;
	m_fFogTarget = 0.0f;
	m_bChangeFog = false;
	m_bChangeColor = false;
	m_bActive = false;

	m_pMembrane = nullptr;
}

//=============================================================================
//　デストラクタ
//=============================================================================
CRenderer::~CRenderer()
{

} 

//=============================================================================
// 初期化
//=============================================================================
HRESULT CRenderer::Init(HWND hWnd, bool bWindow)
{
	D3DPRESENT_PARAMETERS d3dpp;
	D3DDISPLAYMODE d3ddm;

	// Direct3Dオブジェクトの作成
	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pD3D == nullptr)
	{
		return E_FAIL;
	}

	// 現在のディスプレイモードを取得
	if (FAILED(m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
	{
		return E_FAIL;
	}

	// デバイスのプレゼンテーションパラメータの設定
	ZeroMemory(&d3dpp, sizeof(d3dpp));								// ワークをゼロクリア
	d3dpp.BackBufferCount = 1;										// バックバッファの数
	d3dpp.BackBufferWidth = SCREEN_WIDTH;							// ゲーム画面サイズ(幅)
	d3dpp.BackBufferHeight = SCREEN_HEIGHT;							// ゲーム画面サイズ(高さ)
	d3dpp.BackBufferFormat = d3ddm.Format;							// カラーモードの指定
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;						// 映像信号に同期してフリップする
	d3dpp.EnableAutoDepthStencil = TRUE;							// デプスバッファ（Ｚバッファ）とステンシルバッファを作成
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;					// デプスバッファとして24bit、ステンシルバッファとして8ビットを使う
	d3dpp.Windowed = bWindow;										// ウィンドウモード
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;		// リフレッシュレート
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;		// インターバル

	// ディスプレイアダプタを表すためのデバイスを作成
	// 描画と頂点処理をハードウェアで行なう
	if ((FAILED(m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDevice))) &&
		// 上記の設定が失敗したら
		// 描画をハードウェアで行い、頂点処理はCPUで行なう
		(FAILED(m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDevice))) &&
		// 上記の設定が失敗したら
		// 描画と頂点処理をCPUで行なう
		(FAILED(m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDevice))))
	{
		// 生成失敗
		return E_FAIL;
	}

	//レンダーステートの設定
	m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	m_pD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	m_pD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	//サンプラーステートの設定
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

	//テクスチャステージステートの設定
	m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	m_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

	//フォグの有効設定
	m_pD3DDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);

	//フォグカラーの設定
	m_pD3DDevice->SetRenderState(D3DRS_FOGCOLOR, ColorWhite);

	//フォグモード(範囲指定: D3DFOG_LINEAR, 密度指定: D3DFOG_EXP)
	m_pD3DDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);

	//範囲指定
	float fFogStart = DEFAULT_FOG_NEAR, fFogEnd = DEFAULT_FOG_FAR;
	m_fFogNear = DEFAULT_FOG_NEAR;
	m_fFogFar = DEFAULT_FOG_FAR;
	m_fFogTarget = DEFAULT_FOG_NEAR;
	m_pD3DDevice->SetRenderState(D3DRS_FOGSTART, *(DWORD*)(&fFogStart));
	m_pD3DDevice->SetRenderState(D3DRS_FOGEND, *(DWORD*)(&fFogEnd));
	m_targetCol = ColorWhite;
	m_fogColor = ColorWhite;

	m_pMembrane = new CMembraneShading(m_pD3DDevice);

	m_pMembrane->Load();

	m_pPhong = new CPhongShading(m_pD3DDevice);

	m_pPhong->Load();


#ifdef _DEBUG
	// デバッグ情報表示用フォントの生成
	D3DXCreateFont(m_pD3DDevice, 18, 0, 0, 0, FALSE, SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Terminal"), &m_pFont);
#endif

	return S_OK;
}

//=============================================================================
// 終了
//=============================================================================
void CRenderer::Uninit()
{

#ifdef _DEBUG
	// デバッグ情報表示用フォントの破棄
	if (m_pFont != nullptr)
	{
		m_pFont->Release();
		m_pFont = nullptr;
	}
#endif // _DEBUG

	// デバイスの破棄
	if (m_pD3DDevice != nullptr)
	{
		m_pD3DDevice->Release();
		m_pD3DDevice = nullptr;
	}

	// Direct3Dオブジェクトの破棄
	if (m_pD3D != nullptr)
	{
		m_pD3D->Release();
		m_pD3D = nullptr;
	}

	//シェーダーのエフェクトの破棄処理
	if (m_pMembrane)
	{
		m_pMembrane->Invalidate();
		delete m_pMembrane;
		m_pMembrane = nullptr;
	}
	if (m_pPhong)
	{
		m_pPhong->Invalidate();
		delete m_pPhong;
		m_pPhong = nullptr;
	}
}

//=============================================================================
// 更新
//=============================================================================
void CRenderer::Update()
{
	UpdateFog();

	CObject::UpdateAll();			//オブジェクトの更新処理
}

//=============================================================================
// 描画
//=============================================================================
void CRenderer::Draw()
{
	//画面クリア(ステンシルバッファ、バックバッファ＆Zバッファのクリア)
	m_pD3DDevice->Clear(0, NULL, (D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);

	//描画開始
	if (SUCCEEDED(m_pD3DDevice->BeginScene()))
	{//描画開始が成功した場合
	 // ポリゴンの描画処理

		CApplication::GetCamera()->Set();

		CObject::DrawAll();

		//CFade* pFade = CApplication::GetFade();
		//if (pFade != nullptr)
		//{
		//	//フェードが読み込まれていない場合
		//	if (pFade->GetFade() != CFade::FADE_NONE)
		//	{
		//		pFade->Draw();
		//	}
		//}
#ifdef _DEBUG
		// FPS表示
		//DrawFPS();
		CDebugProc::Draw();
		
#endif // _DEBUG

		// Direct3Dによる描画の終了
		m_pD3DDevice->EndScene();
	}

	// バックバッファとフロントバッファの入れ替え
	m_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

//=============================================================================
// デバイスの取得処理
//=============================================================================
LPDIRECT3DDEVICE9 CRenderer::GetDevice(void)
{
	return m_pD3DDevice;
}

//シェーダーのエフェクトの取得処理
CMembraneShading * CRenderer::GetMembraneEffect(void)
{
	return m_pMembrane;
}

CPhongShading * CRenderer::GetPhongEffect(void)
{
	return m_pPhong;
}

void CRenderer::ChangeFog(void)
{
	m_bChangeFog = !m_bChangeFog;
	m_bActive = true;

	if (m_fFogTarget == DEFAULT_FOG_NEAR)
	{
		m_fFogTarget = DEEP_FOG_NEAR;
		m_fFrameFog.x = (DEEP_FOG_NEAR - m_fFogNear) / (float)DEFAULT_FOG_CHANGE_TIME;
		m_fFrameFog.y = (DEEP_FOG_FAR - m_fFogFar) / (float)DEFAULT_FOG_CHANGE_TIME;
	}
	else
	{
		m_fFogTarget = DEFAULT_FOG_NEAR;

		m_fFrameFog.x = (DEFAULT_FOG_NEAR - m_fFogNear) / (float)DEFAULT_FOG_CHANGE_TIME;
		m_fFrameFog.y = (DEFAULT_FOG_FAR - m_fFogFar) / (float)DEFAULT_FOG_CHANGE_TIME;
	}
}

//フォグの設定処理
void CRenderer::SetDeepFog(const bool bFog)
{
	if (m_bChangeFog != bFog)
	{
		m_bChangeFog = bFog;
		m_bActive = true;

		if (m_fFogTarget == DEFAULT_FOG_NEAR)
		{
			m_fFogTarget = DEEP_FOG_NEAR;
			m_fFrameFog.x = (DEEP_FOG_NEAR - m_fFogNear) / (float)DEFAULT_FOG_CHANGE_TIME;
			m_fFrameFog.y = (DEEP_FOG_FAR - m_fFogFar) / (float)DEFAULT_FOG_CHANGE_TIME;
		}
		else
		{
			m_fFogTarget = DEFAULT_FOG_NEAR;

			m_fFrameFog.x = (DEFAULT_FOG_NEAR - m_fFogNear) / (float)DEFAULT_FOG_CHANGE_TIME;
			m_fFrameFog.y = (DEFAULT_FOG_FAR - m_fFogFar) / (float)DEFAULT_FOG_CHANGE_TIME;
		}
	}
}

//フォグの色の設定処理
void CRenderer::SetFogColor(const D3DXCOLOR col)
{
	m_bChangeColor = true;
	m_targetCol = col;
	m_nCntColorFog = DEFAULT_FOG_CHANGE_TIME;

	m_colorSpeed = (m_targetCol - m_fogColor);
	m_colorSpeed.r /= DEFAULT_FOG_CHANGE_TIME;
	m_colorSpeed.g /= DEFAULT_FOG_CHANGE_TIME;
	m_colorSpeed.b /= DEFAULT_FOG_CHANGE_TIME;
	m_colorSpeed.a = 0.0f;
}

//フォグの更新処理
void CRenderer::UpdateFog(void)
{
	if (m_bActive)
	{
		m_nCntFog++;

		m_fFogNear += m_fFrameFog.x;
		m_fFogFar += m_fFrameFog.y;

		if (m_nCntFog >= DEFAULT_FOG_CHANGE_TIME)
		{
			if (m_fFogTarget == DEFAULT_FOG_NEAR)
			{
				m_fFogNear = DEFAULT_FOG_NEAR;
				m_fFogFar  = DEFAULT_FOG_FAR;
			}
			else
			{
				m_fFogNear = DEEP_FOG_NEAR;
				m_fFogFar  = DEEP_FOG_FAR;
			}

			m_nCntFog = 0;
			m_bActive = false;
		}

		m_pD3DDevice->SetRenderState(D3DRS_FOGSTART, *(DWORD*)(&m_fFogNear));
		m_pD3DDevice->SetRenderState(D3DRS_FOGEND, *(DWORD*)(&m_fFogFar));
	}
	if (m_bChangeColor)
	{
		UpdateFogColor();
	}
}

//フォグの色の更新処理
void CRenderer::UpdateFogColor(void)
{
	m_fogColor += m_colorSpeed;

	//フォグカラーの設定
	m_pD3DDevice->SetRenderState(D3DRS_FOGCOLOR, m_fogColor);

	m_nCntColorFog--;

	if (m_nCntColorFog <= 0)
	{
		m_bChangeColor = false;
	}
}

#ifdef _DEBUG

//=============================================================================
// FPS表示
//=============================================================================
void CRenderer::DrawFPS(void)
{
	int fps = GetFPS();
	RECT rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	TCHAR str[256];

	sprintf(str, _T("FPS : %d\n\n \n"), fps);

	// テキスト描画
	m_pFont->DrawText(NULL, str, -1, &rect, DT_LEFT, D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff));
}

#endif // _DEBUG