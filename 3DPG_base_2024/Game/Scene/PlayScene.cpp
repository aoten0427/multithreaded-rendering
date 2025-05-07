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


constexpr int THREAD_COUNT = 2;


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
	// スレッド終了処理
	{
		std::lock_guard<std::mutex> lock(m_frameMutex);
		m_isRunning = false;
		m_frameReady = false;

		for (int i = 0; i < THREAD_COUNT; i++)
		{
			m_isThradRunning[i] = false;
		}
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

	// リソース解放（正しい順序で）
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

	DX::ThrowIfFailed(
		DirectX::CreateWICTextureFromFile(
			device,
			L"Resources/Textures/Box.png",
			nullptr,
			m_texture.ReleaseAndGetAddressOf(),
			0
		)
	);

	m_shaderSet.vertexShader = ShaderManager::CreateVSShader(device, "ModelVS.cso");
	m_shaderSet.pixelShader = ShaderManager::CreatePSShader(device, "ModelPS.cso");
	m_shaderSet.inputLayout = ShaderManager::CreateInputLayout(device, MODEL_INPUT_LAYOUT, "ModelVS.cso");
	m_shaderSet.cBuffer = ShaderManager::CreateConstantBuffer<CBuff>(device);
	
	int SIZE = 3000;
	// グリッドの大きさを計算（正方形に近い形に配置）
	int gridSize = static_cast<int>(ceil(sqrt(static_cast<float>(SIZE))));
	// モデル間の距離
	float spacing = 2.0f; // 適切な距離に調整してください
	// グリッドの開始位置（中央に配置するためにオフセット）
	float startX = -((gridSize - 1) * spacing) / 2.0f;
	float startZ = -((gridSize - 1) * spacing) / 2.0f;

	/*m_models.resize(SIZE);*/
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

		m_models.emplace_back(std::make_unique<Model3D>());
		m_models[i]->Initialize(device);
		m_models[i]->SetShader(m_shaderSet);
		m_models[i]->LoadModel(m_model.get());
		m_models[i]->LoadTexture(m_texture.Get());
		m_models[i]->SetMatrix(m_worlds[i]);
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

		ExecuteCommaxndLists();

		m_frameReady = false;  // フレーム処理完了をマーク
	}
	
	//for (auto& world : m_worlds)
	//{
	//	m_model->Draw(context, *states, world, view, m_projection);
	//}
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

	// スレッド数に合わせてリソースを確保
	m_deferradContext.resize(thradocunt);
	m_commnds.resize(thradocunt, nullptr);  // nullptrで初期化
	m_isThradRunning.resize(thradocunt, false);  // falseで初期化

	// 遅延コンテキストを作成
	for (int i = 0; i < thradocunt; i++)
	{
		device->CreateDeferredContext(0, &m_deferradContext[i]);
	}

	// スレッド実行フラグを設定
	m_isRunning = true;
	m_frameReady = false;
	m_threadsCompleted = 0;

	// スレッドを起動
	m_thrads.clear();  // 念のため初期化
	for (int i = 0; i < thradocunt; i++)
	{
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
			m_frameStartCV.wait(lock, [this, thradindex] {
				return (m_isThradRunning[thradindex] && m_frameReady) || !m_isRunning;
				});

			if (!m_isRunning) break;
		}

		// スレッドごとの担当範囲を計算
		size_t modelCount = m_worlds.size();
		size_t modelsPerThread = (modelCount + THREAD_COUNT - 1) / THREAD_COUNT;
		size_t startIndex = thradindex * modelsPerThread;
		size_t endIndex = std::min(startIndex + modelsPerThread, modelCount);

		// ビュー行列はコピーして使用（スレッドごとにコピー）
		auto states = m_commonResources->GetCommonStates();
		Matrix view = m_debugCamera->GetViewMatrix();
		// 描画処理（ロックする部分を最小化）
		for (size_t i = startIndex; i < endIndex; i++)
		{
			m_models[i]->Render(m_deferradContext[thradindex], states, view, m_projection);
		}

		// コマンドリストを生成
		ID3D11CommandList* command = nullptr;
		auto hr = m_deferradContext[thradindex]->FinishCommandList(false, &command);
		if (SUCCEEDED(hr) && command)
		{
			std::lock_guard<std::mutex> lock(m_commandListMutex);
			if (m_commnds[thradindex]) {
				m_commnds[thradindex]->Release();
			}
			m_commnds[thradindex] = command;
		}

		// 自分のスレッドの実行完了をマーク
		{
			std::lock_guard<std::mutex> lock(m_frameMutex);
			m_isThradRunning[thradindex] = false;

			// アトミック変数はインクリメントのみロック不要
			if (++m_threadsCompleted == THREAD_COUNT) {
				m_frameEndCV.notify_one();
			}
		}
	}
}