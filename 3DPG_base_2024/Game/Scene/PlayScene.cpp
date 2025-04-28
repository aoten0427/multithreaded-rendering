/*
	@file	PlayScene.cpp
	@brief	�v���C�V�[���N���X
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



//---------------------------------------------------------
// �R���X�g���N�^
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
// �f�X�g���N�^
//---------------------------------------------------------
PlayScene::~PlayScene()
{
	// do nothing.

	
}

//---------------------------------------------------------
// ����������
//---------------------------------------------------------
void PlayScene::Initialize(CommonResources* resources)
{
	assert(resources);
	m_commonResources = resources;

	auto device = m_commonResources->GetDeviceResources()->GetD3DDevice();
	auto context = m_commonResources->GetDeviceResources()->GetD3DDeviceContext();
	auto states = m_commonResources->GetCommonStates();


	// �f�o�b�O�J�������쐬����
	RECT rect{ m_commonResources->GetDeviceResources()->GetOutputSize() };
	m_debugCamera = std::make_unique<mylib::DebugCamera>();
	m_debugCamera->Initialize(rect.right, rect.bottom);

	// �ˉe�s����쐬����
	m_projection = SimpleMath::Matrix::CreatePerspectiveFieldOfView(
		XMConvertToRadians(45.0f),
		static_cast<float>(rect.right) / static_cast<float>(rect.bottom),
		0.1f, 1000.0f
	);

	// �摜�̍쐬
	
	DX::ThrowIfFailed(
		DirectX::CreateWICTextureFromFile(
			device,
			L"Resources/Textures/Box.png",
			nullptr,
			m_tex.ReleaseAndGetAddressOf(),
			0
		)
	);

	// ���f����ǂݍ��ޏ���
	std::unique_ptr<DirectX::EffectFactory> fx = std::make_unique<DirectX::EffectFactory>(device);
	fx->SetDirectory(L"Resources/Models");

	// ���f����ǂݍ���
	m_model = DirectX::Model::CreateFromCMO(device, L"Resources/Models/box.cmo", *fx);

	m_instanceSet.vertexShader = ShaderManager::CreateVSShader(device, "InstanceModelVS.cso");
	m_instanceSet.pixelShader = ShaderManager::CreatePSShader(device, "InstanceModelPS.cso");
	m_instanceSet.inputLayout = ShaderManager::CreateInputLayout(device, INSTANCE_INPUT_LAYOUT, "InstanceModelVS.cso");

	// �C���X�^���X�o�b�t�@�̍쐬
	D3D11_BUFFER_DESC instanceBufferDesc = {};
	instanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	instanceBufferDesc.ByteWidth = sizeof(Matrix) * MAX_INSTANCE;
	instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; // ���̕������d�v�I
	instanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	instanceBufferDesc.MiscFlags = 0;
	instanceBufferDesc.StructureByteStride = 0;

	HRESULT hr = device->CreateBuffer(&instanceBufferDesc, nullptr, &m_instanceSet.cBuffer);
	if (FAILED(hr)) {
		// �G���[����
	}

	cBuffer = ShaderManager::CreateConstantBuffer<PNTStaticConstantBuffer>(device);
}

//---------------------------------------------------------
// �X�V����
//---------------------------------------------------------
void PlayScene::Update(float elapsedTime)
{
	UNREFERENCED_PARAMETER(elapsedTime);

	// �f�o�b�O�J�������X�V����
	m_debugCamera->Update(m_commonResources->GetInputManager());
}

//---------------------------------------------------------
// �`�悷��
//---------------------------------------------------------
void PlayScene::Render()
{
	auto device = m_commonResources->GetDeviceResources()->GetD3DDevice();
	auto context = m_commonResources->GetDeviceResources()->GetD3DDeviceContext();
	auto states = m_commonResources->GetCommonStates();

	// �r���[�s����擾����
	const Matrix& view = m_debugCamera->GetViewMatrix();

	
	using namespace DirectX::SimpleMath;
	//�������@��]��
	//�������@��]�p�x
	//Y����90�x��]

	Matrix mat = Matrix::Identity;
	// �O���b�h�̑傫�����v�Z�i�����`�ɋ߂��`�ɔz�u�j
	int gridSize = static_cast<int>(ceil(sqrt(static_cast<float>(MAX_INSTANCE))));
	// ���f���Ԃ̋���
	float spacing = 2.0f; // �K�؂ȋ����ɒ������Ă�������
	// �O���b�h�̊J�n�ʒu�i�����ɔz�u���邽�߂ɃI�t�Z�b�g�j
	float startX = -((gridSize - 1) * spacing) / 2.0f;
	float startZ = -((gridSize - 1) * spacing) / 2.0f;


	for (int i = 0; i < MAX_INSTANCE; i++)
	{
		// �O���b�h���̈ʒu���v�Z
		int row = i / gridSize;
		int col = i % gridSize;

		// XZ���ʏ�̈ʒu���v�Z
		float x = startX + col * spacing;
		float z = startZ + row * spacing;

		// Y����0�i���ʏ�j
		float y = 0.0f;

		// �e�C���X�^���X�̃��[���h�s���ݒ�
		mat = (Matrix::CreateTranslation(Vector3(x, y, z)));
		m_model->Draw(context, *states, mat, view, m_projection);
	}
	return;


	D3D11_MAPPED_SUBRESOURCE mappedResource;
	// �萔�o�b�t�@���}�b�v����
	context->Map(m_instanceSet.cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	CBuff* cb = static_cast<CBuff*>(mappedResource.pData);

	
	for (int i = 0; i < MAX_INSTANCE; i++)
	{
		// �O���b�h���̈ʒu���v�Z
		int row = i / gridSize;
		int col = i % gridSize;

		// XZ���ʏ�̈ʒu���v�Z
		float x = startX + col * spacing;
		float z = startZ + row * spacing;

		// Y����0�i���ʏ�j
		float y = 0.0f;

		// �e�C���X�^���X�̃��[���h�s���ݒ�
		cb->mat[i] = XMMatrixTranspose(Matrix::CreateTranslation(Vector3(x, y, z)));
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
			//�C���f�b�N�X�o�b�t�@�̃Z�b�g
			context->IASetIndexBuffer(mesh->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			//�`����@�i3�p�`�j
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//�V�F�[�_�̐ݒ�
			context->VSSetShader(m_instanceSet.vertexShader.Get(), nullptr, 0);
			context->PSSetShader(m_instanceSet.pixelShader.Get(), nullptr, 0);
			//�C���v�b�g���C�A�E�g�̐ݒ�
			context->IASetInputLayout(m_instanceSet.inputLayout.Get());

			//�u�����h�X�e�[�g
		//�����������Ȃ�
			context->OMSetBlendState(states->Opaque(), nullptr, 0xffffffff);
			//�f�v�X�X�e���V���X�e�[�g
			context->OMSetDepthStencilState(states->DepthDefault(), 0);
			//�e�N�X�`���ƃT���v���[�̐ݒ�
			ID3D11ShaderResourceView* pNull[1] = { 0 };
			context->PSSetShaderResources(0, 1, m_tex.GetAddressOf());
			ID3D11SamplerState* pSampler = states->LinearWrap();;
			context->PSSetSamplers(0, 1, &pSampler);
			//���X�^���C�U�X�e�[�g�i�\�ʕ`��j
			context->RSSetState(states->CullNone());

			//�R���X�^���g�o�b�t�@�̏���
			PNTStaticConstantBuffer sb;
			sb.World = Matrix::Identity;
			sb.View = view.Transpose();
			sb.Projection = m_projection.Transpose();
			//���C�e�B���O
			Vector4 LightDir(0.5f, -1.0f, 0.5f, 0.0f);
			LightDir.Normalize();
			sb.LightDir = LightDir;
			//�f�B�t���[�Y
			sb.Diffuse = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
			//�G�~�b�V�u���Z�B
			sb.Emissive = Vector4(0.4f, 0.4f, 0.4f, 0);
			//�R���X�^���g�o�b�t�@�̍X�V
			context->UpdateSubresource(cBuffer.Get(), 0, nullptr, &sb, 0, 0);
			//�R���X�^���g�o�b�t�@�̐ݒ�
			ID3D11Buffer* pConstantBuffer = cBuffer.Get();
			ID3D11Buffer* pNullConstantBuffer = nullptr;
			//���_�V�F�[�_�ɓn��
			context->VSSetConstantBuffers(0, 1, &pConstantBuffer);
			//�s�N�Z���V�F�[�_�ɓn��
			context->PSSetConstantBuffers(0, 1, &pConstantBuffer);
			//�`��
			context->DrawIndexedInstanced(mesh->indexCount, MAX_INSTANCE, 0, 0, 0);
		}
	}


	
}

//---------------------------------------------------------
// ��n������
//---------------------------------------------------------
void PlayScene::Finalize()
{
  
}

//---------------------------------------------------------
// ���̃V�[��ID���擾����
//---------------------------------------------------------
IScene::SceneID PlayScene::GetNextSceneID() const
{
	// �V�[���ύX������ꍇ
	if (m_isChangeScene)
	{
		return IScene::SceneID::TITLE;
	}

	// �V�[���ύX���Ȃ��ꍇ
	return IScene::SceneID::NONE;
}



