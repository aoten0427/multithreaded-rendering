/*
	@file	PlayScene.h
	@brief	プレイシーンクラス
*/
#pragma once
#include "IScene.h"
#include <thread>
#include <mutex>

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

	
	// マルチスレッドレンダリング用の構造体
	struct RenderCommand
	{
		std::function<void(ID3D11DeviceContext*)> renderFunc;
	};

    struct ThreadData
    {
        ID3D11DeviceContext* deferredContext;
        ID3D11CommandList* commandList;
        std::vector<RenderCommand> commands;
        std::unique_ptr<std::thread> thread;  // threadをユニークポインタに変更
        std::mutex mutex;
        std::atomic<bool> working;

        // デフォルトコンストラクタを明示的に定義
        ThreadData() : deferredContext(nullptr), commandList(nullptr), working(false) {}

        // ムーブコンストラクタを定義
        ThreadData(ThreadData&& other) noexcept
            : deferredContext(other.deferredContext)
            , commandList(other.commandList)
            , commands(std::move(other.commands))
            , thread(std::move(other.thread))
            , working(other.working.load())
        {
            other.deferredContext = nullptr;
            other.commandList = nullptr;
        }

        // コピーを禁止
        ThreadData(const ThreadData&) = delete;
        ThreadData& operator=(const ThreadData&) = delete;

        // ムーブ代入演算子を定義
        ThreadData& operator=(ThreadData&& other) noexcept
        {
            if (this != &other)
            {
                deferredContext = other.deferredContext;
                commandList = other.commandList;
                commands = std::move(other.commands);
                thread = std::move(other.thread);
                working = other.working.load();

                other.deferredContext = nullptr;
                other.commandList = nullptr;
            }
            return *this;
        }
    };

	// マルチスレッドレンダリング用のメンバー変数
	std::vector<ThreadData> m_threadDatas;
	bool m_useMultithreadedRendering;
	static const int THREAD_COUNT = 4; // スレッド数
    std::unique_ptr<DirectX::BasicEffect> m_basicEffect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
public:
	PlayScene();
	~PlayScene() override;

	void Initialize(CommonResources* resources) override;
	void Update(float elapsedTime)override;
	void Render() override;
	void Finalize() override;

	SceneID GetNextSceneID() const;

    void RenderThread(int threadIndex);
};
