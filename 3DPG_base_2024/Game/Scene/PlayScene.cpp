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


constexpr int THREAD_COUNT = 4;


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


		// �X���b�h�̏I����ҋ@
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

	
	int SIZE = 4;
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

	InitalizeMulti(THREAD_COUNT);
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

	auto render = m_commonResources->GetDeviceResources()->GetRenderTargetView();
	auto viewport = m_commonResources->GetDeviceResources()->GetScreenViewport();
	auto depth = m_commonResources->GetDeviceResources()->GetDepthStencilView();

	for (auto& defcon : m_deferradContext)
	{
		defcon->OMSetRenderTargets(1, &render, depth);
		defcon->RSSetViewports(1, &viewport);
	}

	// �r���[�s����擾����
	const Matrix& view = m_debugCamera->GetViewMatrix();
	
	
	using namespace DirectX::SimpleMath;
	Matrix mat = Matrix::Identity;
	
	

	/*m_model->Draw(m_deferradContext[0], *states, mat, view, m_projection);
	m_deferradContext[0]->FinishCommandList(false, &m_commnds[0]);*/
	// ���[�J�[�X���b�h�ɕ`����J�n����悤�ʒm
	{
		std::lock_guard<std::mutex> lock(m_frameMutex);
		m_threadsCompleted = 0;
		m_frameReady = true;  // �V�����t���[����������
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			m_isThradRunning[i] = true;  // ���ׂẴX���b�h���N����Ԃ�
		}
	}
	m_frameStartCV.notify_all();  // ���ׂẴX���b�h�ɒʒm

	// �S�Ẵ��[�J�[�X���b�h����������̂�҂�
	{
		std::unique_lock<std::mutex> lock(m_frameMutex);
		// ���ׂẴX���b�h����������܂őҋ@
		m_frameEndCV.wait(lock, [this] { return m_threadsCompleted == THREAD_COUNT; });

		// �f�o�b�O���\���i�K�v�ɉ����āj
		auto debug = m_commonResources->GetDebugString();
		debug->AddString(std::to_string(nn).c_str());

		{
			std::lock_guard<std::mutex> lock(m_debugstrMutex);
			AddDebugMessageFormat("%d", nn);
			AddDebugMessage("�`��I��");
		}

		m_frameReady = false;  // �t���[�������������}�[�N
	}
	/*ExecuteCommaxndLists();*/
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
		// ���C���X���b�h����̊J�n�V�O�i����҂�
		{
			std::unique_lock<std::mutex> lock(m_frameMutex);
			// ���̃X���b�h�̎��s�t���O��true�ɂȂ�܂ŁA�܂��͐V�����t���[�������������܂őҋ@
			m_frameStartCV.wait(lock, [this, thradindex] {
				return m_isThradRunning[thradindex] && m_frameReady;
				});

			if (!m_isRunning) break;  // �X���b�h�I���t���O�������Ă���ΏI��
		}

		// �����ŃX���b�h�ŗL�̏������s��
		{
			std::lock_guard<std::mutex> lock(m_debugstrMutex);
			AddDebugMessageFormat("�X���b�h %d: nn = %d", thradindex, nn);
		}

		nn++;  // �J�E���^���C���N�������g

		// �����̃X���b�h�̎��s�������}�[�N
		{
			std::lock_guard<std::mutex> lock(m_frameMutex);
			m_isThradRunning[thradindex] = false;  // ���̃X���b�h�̎��s����

			// �S�X���b�h�̊������`�F�b�N
			if (++m_threadsCompleted == THREAD_COUNT) {
				m_frameEndCV.notify_one();  // ���C���X���b�h�ɒʒm
			}
		}
	}
}