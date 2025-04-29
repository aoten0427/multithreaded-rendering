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


	// ���f����ǂݍ��ޏ���
	std::unique_ptr<DirectX::EffectFactory> fx = std::make_unique<DirectX::EffectFactory>(device);
	fx->SetDirectory(L"Resources/Models");

	// ���f����ǂݍ���
	m_model = DirectX::Model::CreateFromCMO(device, L"Resources/Models/box.cmo", *fx);

	m_instancing.Initialize(device);
	m_instancing.LoadModel("box.cmo", device);
	m_instancing.LoadTexture("Box.png", device);

	m_worlds.reserve(100000);

	int SIZE = 50000;
	// �O���b�h�̑傫�����v�Z�i�����`�ɋ߂��`�ɔz�u�j
	int gridSize = static_cast<int>(ceil(sqrt(static_cast<float>(SIZE))));
	// ���f���Ԃ̋���
	float spacing = 2.0f; // �K�؂ȋ����ɒ������Ă�������
	// �O���b�h�̊J�n�ʒu�i�����ɔz�u���邽�߂ɃI�t�Z�b�g�j
	float startX = -((gridSize - 1) * spacing) / 2.0f;
	float startZ = -((gridSize - 1) * spacing) / 2.0f;
	for (int i = 0; i < SIZE; i++)
	{
		// �O���b�h���̈ʒu���v�Z
		int row = i / gridSize;
		int col = i % gridSize;

		// XZ���ʏ�̈ʒu���v�Z
		float x = startX + col * spacing;
		float z = startZ + row * spacing;

		// Y����0�i���ʏ�j
		float y = 0.0f;

		m_worlds.push_back(Matrix::CreateTranslation(Vector3(x, y, z)));
	}
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

	//Matrix mat = Matrix::Identity;
	//
	//int SIZE = 50000;
	//// �O���b�h�̑傫�����v�Z�i�����`�ɋ߂��`�ɔz�u�j
	//int gridSize = static_cast<int>(ceil(sqrt(static_cast<float>(SIZE))));
	//// ���f���Ԃ̋���
	//float spacing = 2.0f; // �K�؂ȋ����ɒ������Ă�������
	//// �O���b�h�̊J�n�ʒu�i�����ɔz�u���邽�߂ɃI�t�Z�b�g�j
	//float startX = -((gridSize - 1) * spacing) / 2.0f;
	//float startZ = -((gridSize - 1) * spacing) / 2.0f;

	//for (int i = 0; i < MAX_INSTANCE; i++)
	//{
	//	// �O���b�h���̈ʒu���v�Z
	//	int row = i / gridSize;
	//	int col = i % gridSize;

	//	// XZ���ʏ�̈ʒu���v�Z
	//	float x = startX + col * spacing;
	//	float z = startZ + row * spacing;

	//	// Y����0�i���ʏ�j
	//	float y = 0.0f;

	//	// �e�C���X�^���X�̃��[���h�s���ݒ�
	//	mat = (Matrix::CreateTranslation(Vector3(x, y, z)));
	//	m_model->Draw(context, *states, mat, view, m_projection);
	//}
	//return;

	
	/*m_worlds.clear();*/
	
	m_instancing.Render(context, states, view, m_projection,m_worlds);
	


	
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



