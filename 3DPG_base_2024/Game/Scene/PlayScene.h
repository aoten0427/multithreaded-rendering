/*
	@file	PlayScene.h
	@brief	プレイシーンクラス
*/
#pragma once
#include "IScene.h"
#include <thread>
#include <mutex>
#include"Game/ShaderManager.h"
#include"Game/Instancing/Instancing.h"
#include"Game/Model/Model3D.h"
#include"Game/ShaderManager.h"

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
	struct CBuff
	{
		DirectX::SimpleMath::Matrix mat;
	};
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
	//テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
	ShaderSet m_shaderSet;

	std::vector<DirectX::SimpleMath::Matrix> m_worlds;

	//マルチスレッド関係
	// マルチスレッド関係（整理・改善）
	std::vector<ID3D11DeviceContext*> m_deferradContext;
	std::vector<ID3D11CommandList*> m_commnds;
	std::mutex m_commandListMutex;  // コマンドリスト操作専用ミューテックス

	bool m_isRunning = false;  // 初期値を設定
	std::vector<std::thread> m_thrads;

	// 不要なミューテックスを削除（m_drawMutex）
	std::condition_variable m_frameStartCV;
	std::condition_variable m_frameEndCV;
	std::mutex m_frameMutex;

	// アトミック変数として適切に使用
	std::atomic<int> m_threadsCompleted{ 0 };
	std::vector<bool> m_isThradRunning;
	std::atomic<bool> m_frameReady{ false };

	std::mutex m_draw;

	std::vector<std::unique_ptr<Model3D>> m_models;
public:
	PlayScene();
	~PlayScene() override;

	void Initialize(CommonResources* resources) override;
	void Update(float elapsedTime)override;
	void Render() override;
	void Finalize() override;

	SceneID GetNextSceneID() const;

private:
	void InitalizeMulti(int thradocunt);

	void ExecuteCommaxndLists();

	void ThradWork(int thradindex);


private:
	// デバッグ文字列を保存する配列
	std::vector<std::string> m_debugMessages;

	// デバッグメッセージの最大表示数
	static constexpr int MAX_DEBUG_MESSAGES = 20;

	std::mutex m_debugstrMutex;


	void AddDebugMessage(const std::string& message)
	{
		// スレッドセーフでない場合は必要に応じてミューテックスなどを使用

		// メッセージを追加
		m_debugMessages.push_back(message);

		// 最大表示数を超えたら古いメッセージを削除
		if (m_debugMessages.size() > MAX_DEBUG_MESSAGES)
		{
			m_debugMessages.erase(m_debugMessages.begin());
		}

		// Visual Studioのデバッグウィンドウにも出力
		OutputDebugStringA((message + "\n").c_str());
	}


	template<typename... Args>
	void AddDebugMessageFormat(const char* format, Args... args)
	{
		// バッファサイズを設定
		constexpr size_t bufferSize = 256;
		char buffer[bufferSize];

		// フォーマット文字列を作成
		snprintf(buffer, bufferSize, format, args...);

		// メッセージとして追加
		AddDebugMessage(buffer);
	}
};
