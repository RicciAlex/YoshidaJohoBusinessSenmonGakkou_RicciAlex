//=============================================================================
//
// firstStage.h
// Author : Ricci Alex
//
//=============================================================================
#ifndef _FIRST_STAGE_H
#define _FIRST_STAGE_H

//=============================================================================
//インクルードファイル
//=============================================================================
#include "game.h"

//=============================================================================
//前方宣言
//=============================================================================

class CFirstStage : public CGame
{
public:
	CFirstStage();								//コンストラクタ
	~CFirstStage() override;					//デストラクタ
												
	HRESULT Init(void);							//初期化処理
	void Uninit(void);							//終了処理
	void Update(void);							//更新処理
												
	static CFirstStage* Create(void);			//生成処理

private:

	static const int DEFAULT_MIN_SOUND_DELAY = 180;
	static const int DEFAULT_MAX_SOUND_DELAY = 400;

	int m_nCntSound;							//サウンドエフェクト用のカウンター
	int m_nSoundDelay;							//サウンドエフェクトのディレイ

};


#endif