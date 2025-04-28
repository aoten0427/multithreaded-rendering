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
		0.1f, 100.0f
	);


	// ���f����ǂݍ��ޏ���
	std::unique_ptr<DirectX::EffectFactory> fx = std::make_unique<DirectX::EffectFactory>(device);
	fx->SetDirectory(L"Resources/Models");

	// ���f����ǂݍ���
	m_model = DirectX::Model::CreateFromCMO(device, L"Resources/Models/dice.cmo", *fx);

	// �x�[�V�b�N�G�t�F�N�g���쐬����
	m_basicEffect = std::make_unique<BasicEffect>(device);
	m_basicEffect->SetVertexColorEnabled(true);
	m_basicEffect->SetLightingEnabled(false);
	m_basicEffect->SetTextureEnabled(false);

	// ���̓��C�A�E�g���쐬����
	DX::ThrowIfFailed(
		CreateInputLayoutFromEffect<VertexPositionColor>(
			device,
			m_basicEffect.get(),
			m_inputLayout.ReleaseAndGetAddressOf()
		)
	);

	// �}���`�X���b�h�����_�����O�̏�����
	if (m_useMultithreadedRendering)
	{
		for (int i = 0; i < THREAD_COUNT; i++)
		{
			m_threadDatas.emplace_back();
			// �x���R���e�L�X�g�̍쐬
			resources->GetDeviceResources()->GetD3DDevice()->CreateDeferredContext(0, &m_threadDatas[i].deferredContext);
			m_threadDatas[i].commandList = nullptr;
			m_threadDatas[i].working = false;
		}
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
	auto context = m_commonResources->GetDeviceResources()->GetD3DDeviceContext();
	auto states = m_commonResources->GetCommonStates();

	// �r���[�s����擾����
	const Matrix& view = m_debugCamera->GetViewMatrix();

	
	using namespace DirectX::SimpleMath;
	//�������@��]��
	//�������@��]�p�x
	//Y����90�x��]

	Matrix mat = Matrix::Identity;

	//// ���f����`�悷��
	//m_model->Draw(context, *states, mat, view, m_projection, false, [&]() {
	//		
	//	});


	// �f�o�b�O�����uDebugString�v�ŕ\������
	auto debugString = m_commonResources->GetDebugString();
	debugString->AddString("Play Scene");


    if (m_useMultithreadedRendering)
    {
        // ���f���̑������擾
		size_t modelCount = 10;
        size_t modelsPerThread = modelCount / THREAD_COUNT;

        // �����_�����O�^�X�N�𕪊�
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            m_threadDatas[i].commands.clear();

            // ���̃X���b�h�ŏ������郂�f���͈̔͂�����
            size_t startIndex = i * modelsPerThread;
            size_t endIndex = (i == THREAD_COUNT - 1) ? modelCount : (i + 1) * modelsPerThread;

            // �e���f���̕`��R�}���h��ǉ�
            for (size_t j = startIndex; j < endIndex; j++)
            {
                // ���[���h�s��Ȃǂ̌v�Z
				DirectX::SimpleMath::Matrix world = Matrix::Identity;

                /*m_model->Draw(context, *states, world, view, m_projection);*/
                //// �`��R�}���h�������_���ŃL���v�`��
                m_threadDatas[i].commands.push_back(RenderCommand{
                    [&](ID3D11DeviceContext* ctx) {
                        // ���f���̕`��
                        m_model->Draw(ctx, *states, world,view, m_projection);
                    }
                    });
            }

            // �X���b�h���J�n
            m_threadDatas[i].working = true;
            if (m_threadDatas[i].thread)
            {
                m_threadDatas[i].thread = nullptr;  // �O�̃X���b�h���N���A
            }
            m_threadDatas[i].thread = std::make_unique<std::thread>(&PlayScene::RenderThread, this, i);
        }

        // �S�X���b�h�̊�����҂�
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            if (m_threadDatas[i].thread && m_threadDatas[i].thread->joinable())
            {
                m_threadDatas[i].thread->join();
            }
        }

        // �R�}���h���X�g�����s
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
        //// �V���O���X���b�h�����_�����O�i�����̏����j
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
// ��n������
//---------------------------------------------------------
void PlayScene::Finalize()
{
	// do nothing.
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

void PlayScene::RenderThread(int threadIndex)
{
    ThreadData& threadData = m_threadDatas[threadIndex];
    ID3D11DeviceContext* context = threadData.deferredContext;

    // ���̃X���b�h�Ɋ��蓖�Ă�ꂽ�R�}���h�����s
    for (const auto& command : threadData.commands)
    {
        command.renderFunc(context);
    }

    // �R�}���h���X�g���쐬
    threadData.deferredContext->FinishCommandList(FALSE, &threadData.commandList);
    threadData.working = false;
}
