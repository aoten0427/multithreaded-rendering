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


constexpr int THREAD_COUNT = 2;


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
	// �X���b�h�I������
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

	// �X���b�h�̏I����ҋ@
	for (auto& thread : m_thrads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	// ���\�[�X����i�����������Łj
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
	// �O���b�h�̑傫�����v�Z�i�����`�ɋ߂��`�ɔz�u�j
	int gridSize = static_cast<int>(ceil(sqrt(static_cast<float>(SIZE))));
	// ���f���Ԃ̋���
	float spacing = 2.0f; // �K�؂ȋ����ɒ������Ă�������
	// �O���b�h�̊J�n�ʒu�i�����ɔz�u���邽�߂ɃI�t�Z�b�g�j
	float startX = -((gridSize - 1) * spacing) / 2.0f;
	float startZ = -((gridSize - 1) * spacing) / 2.0f;

	/*m_models.resize(SIZE);*/
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

		ExecuteCommaxndLists();

		m_frameReady = false;  // �t���[�������������}�[�N
	}
	
	//for (auto& world : m_worlds)
	//{
	//	m_model->Draw(context, *states, world, view, m_projection);
	//}
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

	// �X���b�h���ɍ��킹�ă��\�[�X���m��
	m_deferradContext.resize(thradocunt);
	m_commnds.resize(thradocunt, nullptr);  // nullptr�ŏ�����
	m_isThradRunning.resize(thradocunt, false);  // false�ŏ�����

	// �x���R���e�L�X�g���쐬
	for (int i = 0; i < thradocunt; i++)
	{
		device->CreateDeferredContext(0, &m_deferradContext[i]);
	}

	// �X���b�h���s�t���O��ݒ�
	m_isRunning = true;
	m_frameReady = false;
	m_threadsCompleted = 0;

	// �X���b�h���N��
	m_thrads.clear();  // �O�̂��ߏ�����
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
		// ���C���X���b�h����̊J�n�V�O�i����҂�
		{
			std::unique_lock<std::mutex> lock(m_frameMutex);
			m_frameStartCV.wait(lock, [this, thradindex] {
				return (m_isThradRunning[thradindex] && m_frameReady) || !m_isRunning;
				});

			if (!m_isRunning) break;
		}

		// �X���b�h���Ƃ̒S���͈͂��v�Z
		size_t modelCount = m_worlds.size();
		size_t modelsPerThread = (modelCount + THREAD_COUNT - 1) / THREAD_COUNT;
		size_t startIndex = thradindex * modelsPerThread;
		size_t endIndex = std::min(startIndex + modelsPerThread, modelCount);

		// �r���[�s��̓R�s�[���Ďg�p�i�X���b�h���ƂɃR�s�[�j
		auto states = m_commonResources->GetCommonStates();
		Matrix view = m_debugCamera->GetViewMatrix();
		// �`�揈���i���b�N���镔�����ŏ����j
		for (size_t i = startIndex; i < endIndex; i++)
		{
			m_models[i]->Render(m_deferradContext[thradindex], states, view, m_projection);
		}

		// �R�}���h���X�g�𐶐�
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

		// �����̃X���b�h�̎��s�������}�[�N
		{
			std::lock_guard<std::mutex> lock(m_frameMutex);
			m_isThradRunning[thradindex] = false;

			// �A�g�~�b�N�ϐ��̓C���N�������g�̂݃��b�N�s�v
			if (++m_threadsCompleted == THREAD_COUNT) {
				m_frameEndCV.notify_one();
			}
		}
	}
}