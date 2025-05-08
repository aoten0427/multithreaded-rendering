#include"pch.h"
#include"Instancing.h"

const std::vector<D3D11_INPUT_ELEMENT_DESC> INSTANCE_INPUT_LAYOUT =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "MATRIX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "MATRIX", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "MATRIX", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	{ "MATRIX", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
};

Instancing::Instancing()
{
}



void Instancing::Initialize(ID3D11Device* device,int maxinstance)
{
	m_maxInstance = std::min(maxinstance, MAX_INSTANCE); // 上限を設定

	m_instanceSet.vertexShader = ShaderManager::CreateVSShader(device, "InstanceModelVS.cso");
	m_instanceSet.pixelShader = ShaderManager::CreatePSShader(device, "InstanceModelPS.cso");
	m_instanceSet.inputLayout = ShaderManager::CreateInputLayout(device, INSTANCE_INPUT_LAYOUT, "InstanceModelVS.cso");

	// インスタンスバッファの作成
	D3D11_BUFFER_DESC instanceBufferDesc = {};
	instanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	instanceBufferDesc.ByteWidth = sizeof(DirectX::SimpleMath::Matrix) * m_maxInstance;
	instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	instanceBufferDesc.MiscFlags = 0;
	instanceBufferDesc.StructureByteStride = 0;
	device->CreateBuffer(&instanceBufferDesc, nullptr, &m_instanceSet.cBuffer);

	cBuffer = ShaderManager::CreateConstantBuffer<PNTStaticConstantBuffer>(device);

	
}

void Instancing::LoadModel(std::string filename, ID3D11Device* device)
{
	std::string folderPath = "Resources/Models/";

	std::unique_ptr<DirectX::EffectFactory> fx = std::make_unique<DirectX::EffectFactory>(device);
	std::wstring myWideString = std::wstring(folderPath.begin(), folderPath.end());
	const wchar_t* folder = myWideString.c_str();
	fx->SetDirectory(folder);

	folderPath += filename;
	myWideString = std::wstring(folderPath.begin(), folderPath.end());
	folder = myWideString.c_str();
	m_model = DirectX::Model::CreateFromCMO(device, folder, *fx);
}

void Instancing::LoadTexture(std::string filename, ID3D11Device* device)
{
	std::string folderPath = "Resources/Textures/" + filename;
	std::wstring myWideString = std::wstring(folderPath.begin(), folderPath.end());
	const wchar_t* folder = myWideString.c_str();

	DX::ThrowIfFailed(
		DirectX::CreateWICTextureFromFile(
			device,
			folder,
			nullptr,
			m_texture.ReleaseAndGetAddressOf(),
			0
		)
	);
}

void Instancing::Render(ID3D11DeviceContext* context, DirectX::DX11::CommonStates* states,const DirectX::SimpleMath::Matrix& view,const DirectX::SimpleMath::Matrix& projection, std::vector<DirectX::SimpleMath::Matrix>& worlds)
{
	if (worlds.empty())return;
	if (MAX_INSTANCE < worlds.size())return;

	using namespace DirectX::SimpleMath;

	// 実際に描画するインスタンス数を決定
	int instanceCount = static_cast<int>(std::min(worlds.size(), static_cast<size_t>(m_maxInstance)));

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	// 定数バッファをマップする
	context->Map(m_instanceSet.cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	CBuff* cb = static_cast<CBuff*>(mappedResource.pData);
	for (int i = 0; i < instanceCount; i++) {
		cb->mat[i] = worlds[i].Transpose();
	}
	context->Unmap(m_instanceSet.cBuffer.Get(), 0);

	for (auto& meshes : m_model->meshes)
	{
		for (auto& mesh : meshes->meshParts)
		{
			ID3D11Buffer* pBuf[2] = { mesh->vertexBuffer.Get(), m_instanceSet.cBuffer.Get() };

			UINT strides[2] = { mesh->vertexStride, sizeof(Matrix) };
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
			context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
			ID3D11SamplerState* pSampler = states->LinearWrap();;
			context->PSSetSamplers(0, 1, &pSampler);
			//ラスタライザステート（表面描画）
			context->RSSetState(states->CullNone());

			//コンスタントバッファの準備
			PNTStaticConstantBuffer sb;
			sb.World = Matrix::Identity;
			sb.View = view.Transpose();
			sb.Projection = projection.Transpose();
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
			context->DrawIndexedInstanced(mesh->indexCount,instanceCount, 0, 0, 0);
		}
	}
}


void Instancing::Render(ID3D11DeviceContext* context, DirectX::DX11::CommonStates* states, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& projection)
{
	using namespace DirectX::SimpleMath;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	// 定数バッファをマップする
	context->Map(m_instanceSet.cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	CBuff* cb = static_cast<CBuff*>(mappedResource.pData);
	
	int SIZE = 50000;
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

		// 各インスタンスのワールド行列を設定
		cb->mat[i] = (Matrix::CreateTranslation(Vector3(x, y, z))).Transpose();
	}

	context->Unmap(m_instanceSet.cBuffer.Get(), 0);

	for (auto& meshes : m_model->meshes)
	{
		for (auto& mesh : meshes->meshParts)
		{
			ID3D11Buffer* pBuf[2] = { mesh->vertexBuffer.Get(), m_instanceSet.cBuffer.Get() };

			UINT strides[2] = { mesh->vertexStride, sizeof(Matrix) };
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
			context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
			ID3D11SamplerState* pSampler = states->LinearWrap();;
			context->PSSetSamplers(0, 1, &pSampler);
			//ラスタライザステート（表面描画）
			context->RSSetState(states->CullNone());

			//コンスタントバッファの準備
			PNTStaticConstantBuffer sb;
			sb.World = Matrix::Identity;
			sb.View = view.Transpose();
			sb.Projection = projection.Transpose();
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


