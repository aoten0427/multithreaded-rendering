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
	// do nothing.

	
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
		0.1f, 100.0f
	);


	// モデルを読み込む準備
	std::unique_ptr<DirectX::EffectFactory> fx = std::make_unique<DirectX::EffectFactory>(device);
	fx->SetDirectory(L"Resources/Models");

	// モデルを読み込む
	m_model = DirectX::Model::CreateFromCMO(device, L"Resources/Models/dice.cmo", *fx);

	// ベーシックエフェクトを作成する
	m_basicEffect = std::make_unique<BasicEffect>(device);
	m_basicEffect->SetVertexColorEnabled(true);
	m_basicEffect->SetLightingEnabled(false);
	m_basicEffect->SetTextureEnabled(false);

	// 入力レイアウトを作成する
	DX::ThrowIfFailed(
		CreateInputLayoutFromEffect<VertexPositionColor>(
			device,
			m_basicEffect.get(),
			m_inputLayout.ReleaseAndGetAddressOf()
		)
	);

	// マルチスレッドレンダリングの初期化
	if (m_useMultithreadedRendering)
	{
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			m_threadDatas.emplace_back();
			// 遅延コンテキストの作成
			resources->GetDeviceResources()->GetD3DDevice()->CreateDeferredContext(0, &m_threadDatas[i].deferredContext);
			m_threadDatas[i].commandList = nullptr;
			m_threadDatas[i].working = false;
		}
	}
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
	auto context = m_commonResources->GetDeviceResources()->GetD3DDeviceContext();
	auto states = m_commonResources->GetCommonStates();

	// ビュー行列を取得する
	const Matrix& view = m_debugCamera->GetViewMatrix();

	
	using namespace DirectX::SimpleMath;
	//第一引数　回転軸
	//第二引数　回転角度
	//Y軸で90度回転

	Matrix mat = Matrix::Identity;

	//// モデルを描画する
	//m_model->Draw(context, *states, mat, view, m_projection, false, [&]() {
	//		
	//	});


	// デバッグ情報を「DebugString」で表示する
	auto debugString = m_commonResources->GetDebugString();
	debugString->AddString("Play Scene");


    if (m_useMultithreadedRendering)
    {
        // モデルの総数を取得
		size_t modelCount = 10;
        size_t modelsPerThread = modelCount / THREAD_COUNT;

        // レンダリングタスクを分割
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            m_threadDatas[i].commands.clear();

            // このスレッドで処理するモデルの範囲を決定
            size_t startIndex = i * modelsPerThread;
            size_t endIndex = (i == THREAD_COUNT - 1) ? modelCount : (i + 1) * modelsPerThread;

            // 各モデルの描画コマンドを追加
            for (size_t j = startIndex; j < endIndex; j++)
            {
                // ワールド行列などの計算
				DirectX::SimpleMath::Matrix world = Matrix::Identity;

                /*m_model->Draw(context, *states, world, view, m_projection);*/
                //// 描画コマンドをラムダ式でキャプチャ
                m_threadDatas[i].commands.push_back(RenderCommand{
                    [&](ID3D11DeviceContext* ctx) {
                        // モデルの描画
                        m_model->Draw(ctx, *states, world,view, m_projection);
                    }
                    });
            }

            // スレッドを開始
            m_threadDatas[i].working = true;
            if (m_threadDatas[i].thread)
            {
                m_threadDatas[i].thread = nullptr;  // 前のスレッドをクリア
            }
            m_threadDatas[i].thread = std::make_unique<std::thread>(&PlayScene::RenderThread, this, i);
        }

        // 全スレッドの完了を待つ
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            if (m_threadDatas[i].thread && m_threadDatas[i].thread->joinable())
            {
                m_threadDatas[i].thread->join();
            }
        }

        // コマンドリストを実行
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            if (m_threadDatas[i].commandList)
            {
                context->ExecuteCommandList(m_threadDatas[i].commandList, FALSE);
                m_threadDatas[i].commandList->Release();
                m_threadDatas[i].commandList = nullptr;
            }
        }
    }
    else
    {
        //// シングルスレッドレンダリング（既存の処理）
        //for (size_t i = 0; i < m_models.size(); i++)
        //{
        //    DirectX::SimpleMath::Matrix world = CalculateWorldMatrix(i);

        //    context->IASetInputLayout(m_states->inputLayout.Get());
        //    m_effect->SetWorld(world);
        //    m_effect->Apply(context);

        //    m_models[i]->Draw(context, *m_states, world, m_view, m_projection);
        //}
    }
}

//---------------------------------------------------------
// 後始末する
//---------------------------------------------------------
void PlayScene::Finalize()
{
	// do nothing.
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

void PlayScene::RenderThread(int threadIndex)
{
    ThreadData& threadData = m_threadDatas[threadIndex];
    ID3D11DeviceContext* context = threadData.deferredContext;

    // このスレッドに割り当てられたコマンドを実行
    for (const auto& command : threadData.commands)
    {
        command.renderFunc(context);
    }

    // コマンドリストを作成
    threadData.deferredContext->FinishCommandList(FALSE, &threadData.commandList);
    threadData.working = false;
}
