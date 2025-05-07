/*
	@file	PlayScene.cpp
	@brief	プレイシーンクラス
*/
#include "pch.h"
#include "PlayScene.h"
#include "Game/CommonResources.h"
#include "DeviceResources.h"
#include "Libraries/MyLib/DebugCamera.h"
#include "Libraries/MyLib/DebugString.h"
#include "Libraries/MyLib/GridFloor.h"
#include "Libraries/MyLib/InputManager.h"
#include "Libraries/MyLib/MemoryLeakDetector.h"
#include <cassert>

using namespace DirectX;
using namespace DirectX::SimpleMath;


constexpr int THREAD_COUNT = 4;


//---------------------------------------------------------
// コンストラクタ
//---------------------------------------------------------
PlayScene::PlayScene()
	:
	m_commonResources{},
	m_debugCamera{},
	m_projection{},
	m_isChangeScene{},
	m_model{}
{
	
}

//---------------------------------------------------------
// デストラクタ
//---------------------------------------------------------
PlayScene::~PlayScene()
{
	m_isRunning = false;

	// do nothing.
	for (int i = 0;i < THREAD_COUNT;i++)
	{
		{
			std::lock_guard<std::mutex> lock(m_frameMutex);
			m_isThradRunning[i] = false;
		}
		m_isThradRunning[i] = false;
	}
	m_frameStartCV.notify_all();


		// スレッドの終了を待機
	for (auto& thread : m_thrads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}


	for (auto& command : m_commnds)
	{
		if (command)
		{
			command->Release();
			command = nullptr;
		}
	}

	for (auto& context : m_deferradContext)
	{
		if (context)
		{
			context->Release();
			context = nullptr;
		}
	}
}

//---------------------------------------------------------
// 初期化する
//---------------------------------------------------------
void PlayScene::Initialize(CommonResources* resources)
{
	assert(resources);
	m_commonResources = resources;

	auto device = m_commonResources->GetDeviceResources()->GetD3DDevice();
	auto context = m_commonResources->GetDeviceResources()->GetD3DDeviceContext();
	auto states = m_commonResources->GetCommonStates();


	// デバッグカメラを作成する
	RECT rect{ m_commonResources->GetDeviceResources()->GetOutputSize() };
	m_debugCamera = std::make_unique<mylib::DebugCamera>();
	m_debugCamera->Initialize(rect.right, rect.bottom);

	// 射影行列を作成する
	m_projection = SimpleMath::Matrix::CreatePerspectiveFieldOfView(
		XMConvertToRadians(45.0f),
		static_cast<float>(rect.right) / static_cast<float>(rect.bottom),
		0.1f, 1000.0f
	);


	// モデルを読み込む準備
	std::unique_ptr<DirectX::EffectFactory> fx = std::make_unique<DirectX::EffectFactory>(device);
	fx->SetDirectory(L"Resources/Models");

	// モデルを読み込む
	m_model = DirectX::Model::CreateFromCMO(device, L"Resources/Models/box.cmo", *fx);

	
	int SIZE = 4;
	// グリッドの大きさを計算（正方形に近い形に配置）
	int gridSize = static_cast<int>(ceil(sqrt(static_cast<float>(SIZE))));
	// モデル間の距離
	float spacing = 2.0f; // 適切な距離に調整してください
	// グリッドの開始位置（中央に配置するためにオフセット）
	float startX = -((gridSize - 1) * spacing) / 2.0f;
	float startZ = -((gridSize - 1) * spacing) / 2.0f;
	for (int i = 0; i < SIZE; i++)
	{
		// グリッド内の位置を計算
		int row = i / gridSize;
		int col = i % gridSize;

		// XZ平面上の位置を計算
		float x = startX + col * spacing;
		float z = startZ + row * spacing;

		// Y軸は0（平面上）
		float y = 0.0f;

		m_worlds.push_back(Matrix::CreateTranslation(Vector3(x, y, z)));
	}

	InitalizeMulti(THREAD_COUNT);
}

//---------------------------------------------------------
// 更新する
//---------------------------------------------------------
void PlayScene::Update(float elapsedTime)
{
	UNREFERENCED_PARAMETER(elapsedTime);

	// デバッグカメラを更新する
	m_debugCamera->Update(m_commonResources->GetInputManager());
}

//---------------------------------------------------------
// 描画する
//---------------------------------------------------------
void PlayScene::Render()
{
	auto device = m_commonResources->GetDeviceResources()->GetD3DDevice();
	auto context = m_commonResources->GetDeviceResources()->GetD3DDeviceContext();
	auto states = m_commonResources->GetCommonStates();

	auto render = m_commonResources->GetDeviceResources()->GetRenderTargetView();
	auto viewport = m_commonResources->GetDeviceResources()->GetScreenViewport();
	auto depth = m_commonResources->GetDeviceResources()->GetDepthStencilView();

	for (auto& defcon : m_deferradContext)
	{
		defcon->OMSetRenderTargets(1, &render, depth);
		defcon->RSSetViewports(1, &viewport);
	}

	// ビュー行列を取得する
	const Matrix& view = m_debugCamera->GetViewMatrix();
	
	
	using namespace DirectX::SimpleMath;
	Matrix mat = Matrix::Identity;
	
	

	/*m_model->Draw(m_deferradContext[0], *states, mat, view, m_projection);
	m_deferradContext[0]->FinishCommandList(false, &m_commnds[0]);*/
	// ワーカースレッドに描画を開始するよう通知
	{
		std::lock_guard<std::mutex> lock(m_frameMutex);
		m_threadsCompleted = 0;
		m_frameReady = true;  // 新しいフレーム準備完了
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			m_isThradRunning[i] = true;  // すべてのスレッドを起動状態に
		}
	}
	m_frameStartCV.notify_all();  // すべてのスレッドに通知

	// 全てのワーカースレッドが完了するのを待つ
	{
		std::unique_lock<std::mutex> lock(m_frameMutex);
		// すべてのスレッドが完了するまで待機
		m_frameEndCV.wait(lock, [this] { return m_threadsCompleted == THREAD_COUNT; });

		// デバッグ情報表示（必要に応じて）
		auto debug = m_commonResources->GetDebugString();
		debug->AddString(std::to_string(nn).c_str());

		{
			std::lock_guard<std::mutex> lock(m_debugstrMutex);
			AddDebugMessageFormat("%d", nn);
			AddDebugMessage("描画終了");
		}

		m_frameReady = false;  // フレーム処理完了をマーク
	}
	/*ExecuteCommaxndLists();*/
}

//---------------------------------------------------------
// 後始末する
//---------------------------------------------------------
void PlayScene::Finalize()
{
  
}

//---------------------------------------------------------
// 次のシーンIDを取得する
//---------------------------------------------------------
IScene::SceneID PlayScene::GetNextSceneID() const
{
	// シーン変更がある場合
	if (m_isChangeScene)
	{
		return IScene::SceneID::TITLE;
	}

	// シーン変更がない場合
	return IScene::SceneID::NONE;
}

void PlayScene::InitalizeMulti(int thradocunt)
{
	auto device = m_commonResources->GetDeviceResources()->GetD3DDevice();
	m_deferradContext.resize(thradocunt);
	m_commnds.resize(thradocunt);
	for (int i = 0; i < thradocunt; i++)
	{
		device->CreateDeferredContext(false, &m_deferradContext[i]);
	}

	m_isRunning = true;
	for (int i = 0; i < thradocunt; i++)
	{
		m_isThradRunning.push_back(true);
		m_thrads.emplace_back(&PlayScene::ThradWork, this, i);
	}
}

void PlayScene::ExecuteCommaxndLists()
{
	auto context = m_commonResources->GetDeviceResources()->GetD3DDeviceContext();

	for (auto& command : m_commnds)
	{
		if (command)
		{
			context->ExecuteCommandList(command, false);
			command->Release();
			command = nullptr;
		}
	}
}

void PlayScene::ThradWork(int thradindex)
{
	while (m_isRunning)
	{
		// メインスレッドからの開始シグナルを待つ
		{
			std::unique_lock<std::mutex> lock(m_frameMutex);
			// このスレッドの実行フラグがtrueになるまで、または新しいフレームが準備されるまで待機
			m_frameStartCV.wait(lock, [this, thradindex] {
				return m_isThradRunning[thradindex] && m_frameReady;
				});

			if (!m_isRunning) break;  // スレッド終了フラグがたっていれば終了
		}

		// ここでスレッド固有の処理を行う
		{
			std::lock_guard<std::mutex> lock(m_debugstrMutex);
			AddDebugMessageFormat("スレッド %d: nn = %d", thradindex, nn);
		}

		nn++;  // カウンタをインクリメント

		// 自分のスレッドの実行完了をマーク
		{
			std::lock_guard<std::mutex> lock(m_frameMutex);
			m_isThradRunning[thradindex] = false;  // このスレッドの実行完了

			// 全スレッドの完了をチェック
			if (++m_threadsCompleted == THREAD_COUNT) {
				m_frameEndCV.notify_one();  // メインスレッドに通知
			}
		}
	}
}