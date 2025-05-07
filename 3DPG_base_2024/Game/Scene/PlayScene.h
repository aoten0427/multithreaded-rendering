/*
	@file	PlayScene.h
	@brief	�v���C�V�[���N���X
*/
#pragma once
#include "IScene.h"
#include <thread>
#include <mutex>
#include"Game/ShaderManager.h"
#include"Game/Instancing/Instancing.h"
#include"Game/Model/Model3D.h"
#include"Game/ShaderManager.h"

// �O���錾
class CommonResources;

namespace mylib
{
	class DebugCamera;
	class GridFloor;
}


class PlayScene final :
    public IScene
{
private:
	struct CBuff
	{
		DirectX::SimpleMath::Matrix mat;
	};
private:

	// ���ʃ��\�[�X
	CommonResources* m_commonResources;

	// �f�o�b�O�J����
	std::unique_ptr<mylib::DebugCamera> m_debugCamera;

	// �ˉe�s��
	DirectX::SimpleMath::Matrix m_projection;

	// �V�[���`�F���W�t���O
	bool m_isChangeScene;

	// ���f��
	std::unique_ptr<DirectX::Model> m_model;
	//�e�N�X�`��
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
	ShaderSet m_shaderSet;

	std::vector<DirectX::SimpleMath::Matrix> m_worlds;

	//�}���`�X���b�h�֌W
	// �}���`�X���b�h�֌W�i�����E���P�j
	std::vector<ID3D11DeviceContext*> m_deferradContext;
	std::vector<ID3D11CommandList*> m_commnds;
	std::mutex m_commandListMutex;  // �R�}���h���X�g�����p�~���[�e�b�N�X

	bool m_isRunning = false;  // �����l��ݒ�
	std::vector<std::thread> m_thrads;

	// �s�v�ȃ~���[�e�b�N�X���폜�im_drawMutex�j
	std::condition_variable m_frameStartCV;
	std::condition_variable m_frameEndCV;
	std::mutex m_frameMutex;

	// �A�g�~�b�N�ϐ��Ƃ��ēK�؂Ɏg�p
	std::atomic<int> m_threadsCompleted{ 0 };
	std::vector<bool> m_isThradRunning;
	std::atomic<bool> m_frameReady{ false };

	std::mutex m_draw;

	std::vector<std::unique_ptr<Model3D>> m_models;
public:
	PlayScene();
	~PlayScene() override;

	void Initialize(CommonResources* resources) override;
	void Update(float elapsedTime)override;
	void Render() override;
	void Finalize() override;

	SceneID GetNextSceneID() const;

private:
	void InitalizeMulti(int thradocunt);

	void ExecuteCommaxndLists();

	void ThradWork(int thradindex);


private:
	// �f�o�b�O�������ۑ�����z��
	std::vector<std::string> m_debugMessages;

	// �f�o�b�O���b�Z�[�W�̍ő�\����
	static constexpr int MAX_DEBUG_MESSAGES = 20;

	std::mutex m_debugstrMutex;


	void AddDebugMessage(const std::string& message)
	{
		// �X���b�h�Z�[�t�łȂ��ꍇ�͕K�v�ɉ����ă~���[�e�b�N�X�Ȃǂ��g�p

		// ���b�Z�[�W��ǉ�
		m_debugMessages.push_back(message);

		// �ő�\�����𒴂�����Â����b�Z�[�W���폜
		if (m_debugMessages.size() > MAX_DEBUG_MESSAGES)
		{
			m_debugMessages.erase(m_debugMessages.begin());
		}

		// Visual Studio�̃f�o�b�O�E�B���h�E�ɂ��o��
		OutputDebugStringA((message + "\n").c_str());
	}


	template<typename... Args>
	void AddDebugMessageFormat(const char* format, Args... args)
	{
		// �o�b�t�@�T�C�Y��ݒ�
		constexpr size_t bufferSize = 256;
		char buffer[bufferSize];

		// �t�H�[�}�b�g��������쐬
		snprintf(buffer, bufferSize, format, args...);

		// ���b�Z�[�W�Ƃ��Ēǉ�
		AddDebugMessage(buffer);
	}
};
