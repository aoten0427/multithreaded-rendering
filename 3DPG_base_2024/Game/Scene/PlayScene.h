/*
	@file	PlayScene.h
	@brief	プレイシーンクラス
*/
#pragma once
#include "IScene.h"

// 前方宣言
class CommonResources;

namespace mylib
{
	class DebugCamera;
	class GridFloor;
}


class PlayScene final :
    public IScene
{
private:
	// 共通リソース
	CommonResources* m_commonResources;

	// デバッグカメラ
	std::unique_ptr<mylib::DebugCamera> m_debugCamera;

	// 射影行列
	DirectX::SimpleMath::Matrix m_projection;

	// シーンチェンジフラグ
	bool m_isChangeScene;

	// モデル
	std::unique_ptr<DirectX::Model> m_model;

	

public:
	PlayScene();
	~PlayScene() override;

	void Initialize(CommonResources* resources) override;
	void Update(float elapsedTime)override;
	void Render() override;
	void Finalize() override;

	SceneID GetNextSceneID() const;
};
