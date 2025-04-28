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

const std::vector<D3D11_INPUT_ELEMENT_DESC> INSTANCE_INPUT_LAYOUT =
{
	// 通常の頂点データ（スロット0）
	{ "Position", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },

	// インスタンスデータ（スロット1）- 行列は4つのfloat4として定義
	{ "MATRIX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
	{ "MATRIX", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
	{ "MATRIX", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 0 },
	{ "MATRIX", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 0 }
};



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

	m_instanceSet.vertexShader = ShaderManager::CreateVSShader(device, "InstanceModelVS.cso");
	m_instanceSet.pixelShader = ShaderManager::CreatePSShader(device, "InstanceModelPS.cso");
	m_instanceSet.inputLayout = ShaderManager::CreateInputLayout(device, INSTANCE_INPUT_LAYOUT, "InstanceModelVS.cso");

	// インスタンスバッファの作成
	D3D11_BUFFER_DESC instanceBufferDesc = {};
	instanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	instanceBufferDesc.ByteWidth = sizeof(Matrix) * MAX_INSTANCE;
	instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // この部分が重要！
	instanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	instanceBufferDesc.MiscFlags = 0;
	instanceBufferDesc.StructureByteStride = 0;

	HRESULT hr = device->CreateBuffer(&instanceBufferDesc, nullptr, &m_instanceSet.cBuffer);
	if (FAILED(hr)) {
		// エラー処理
	}

	cBuffer = ShaderManager::CreateConstantBuffer<PNTStaticConstantBuffer>(device);
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

	// ビュー行列を取得する
	const Matrix& view = m_debugCamera->GetViewMatrix();

	
	using namespace DirectX::SimpleMath;
	//第一引数　回転軸
	//第二引数　回転角度
	//Y軸で90度回転

	Matrix mat = Matrix::Identity;

  
	//// モデルを描画する
	/*m_model->Draw(context, *states, mat, view, m_projection, false, [&]() {
			
		});*/


	D3D11_MAPPED_SUBRESOURCE mappedResource;
	// 定数バッファをマップする
	context->Map(m_instanceSet.cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	CBuff* cb = static_cast<CBuff*>(mappedResource.pData);
	for (int i = 0; i < MAX_INSTANCE; i++)
	{
		// インスタンスごとに異なる位置を設定
		float x = (i % 10) * 2.0f - 10.0f;
		float y = ((i / 10) % 10) * 2.0f;
		float z = (i / 100) * 2.0f - 10.0f;

		cb->mat[i] = Matrix::CreateTranslation(x, y, z);
	}

	context->Unmap(m_instanceSet.cBuffer.Get(), 0);

	

	for (auto& meshes : m_model->meshes)
	{
		for (auto& mesh : meshes->meshParts)
		{
			ID3D11Buffer* pBuf[2] = { mesh->vertexBuffer.Get(), m_instanceSet.cBuffer.Get() };

			// ここが問題 - strideとoffsetは配列である必要がある
			UINT strides[2] = { mesh->vertexStride, sizeof(Matrix) }; // 正しい頂点ストライドとインスタンスデータサイズ
			UINT offsets[2] = { 0, 0 };

			context->IASetVertexBuffers(0, 2, pBuf, strides, offsets);
			//インデックスバッファのセット
			context->IASetIndexBuffer(mesh->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			//描画方法（3角形）
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//シェーダの設定
			context->VSSetShader(m_instanceSet.vertexShader.Get(), nullptr, 0);
			context->PSSetShader(m_instanceSet.pixelShader.Get(), nullptr, 0);
			//インプットレイアウトの設定
			context->IASetInputLayout(m_instanceSet.inputLayout.Get());

			//ブレンドステート
		//透明処理しない
			context->OMSetBlendState(states->Opaque(), nullptr, 0xffffffff);
			//デプスステンシルステート
			context->OMSetDepthStencilState(states->DepthDefault(), 0);
			//テクスチャとサンプラーの設定
			ID3D11ShaderResourceView* pNull[1] = { 0 };
			/*context->PSSetShaderResources(0, 1, m_TextureResource->GetShaderResourceView().GetAddressOf());*/
			ID3D11SamplerState* pSampler = states->LinearClamp();
			context->PSSetSamplers(0, 1, &pSampler);
			//ラスタライザステート（表面描画）
			context->RSSetState(states->CullNone());

			//コンスタントバッファの準備
			PNTStaticConstantBuffer sb;
			sb.World = Matrix::Identity;	//ワールド行列はダミー
			sb.View = view;
			sb.Projection = m_projection;
			//ライティング
			Vector4 LightDir(0.5f, -1.0f, 0.5f, 0.0f);
			LightDir.Normalize();
			sb.LightDir = LightDir;
			//ディフューズ
			sb.Diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
			//エミッシブ加算。
			sb.Emissive = Vector4(0.4f, 0.4f, 0.4f, 0);
			//コンスタントバッファの更新
			context->UpdateSubresource(cBuffer.Get(), 0, nullptr, &sb, 0, 0);
			//コンスタントバッファの設定
			ID3D11Buffer* pConstantBuffer = cBuffer.Get();
			ID3D11Buffer* pNullConstantBuffer = nullptr;
			//頂点シェーダに渡す
			context->VSSetConstantBuffers(0, 1, &pConstantBuffer);
			//ピクセルシェーダに渡す
			context->PSSetConstantBuffers(0, 1, &pConstantBuffer);
			//描画
			context->DrawIndexedInstanced(mesh->indexCount, MAX_INSTANCE, 0, 0, 0);
		}
	}


	
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

