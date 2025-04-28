/*
	@file	PlayScene.h
	@brief	�v���C�V�[���N���X
*/
#pragma once
#include "IScene.h"
#include <thread>
#include <mutex>

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

	
	// �}���`�X���b�h�����_�����O�p�̍\����
	struct RenderCommand
	{
		std::function<void(ID3D11DeviceContext*)> renderFunc;
	};

    struct ThreadData
    {
        ID3D11DeviceContext* deferredContext;
        ID3D11CommandList* commandList;
        std::vector<RenderCommand> commands;
        std::unique_ptr<std::thread> thread;  // thread�����j�[�N�|�C���^�ɕύX
        std::mutex mutex;
        std::atomic<bool> working;

        // �f�t�H���g�R���X�g���N�^�𖾎��I�ɒ�`
        ThreadData() : deferredContext(nullptr), commandList(nullptr), working(false) {}

        // ���[�u�R���X�g���N�^���`
        ThreadData(ThreadData&& other) noexcept
            : deferredContext(other.deferredContext)
            , commandList(other.commandList)
            , commands(std::move(other.commands))
            , thread(std::move(other.thread))
            , working(other.working.load())
        {
            other.deferredContext = nullptr;
            other.commandList = nullptr;
        }

        // �R�s�[���֎~
        ThreadData(const ThreadData&) = delete;
        ThreadData& operator=(const ThreadData&) = delete;

        // ���[�u������Z�q���`
        ThreadData& operator=(ThreadData&& other) noexcept
        {
            if (this != &other)
            {
                deferredContext = other.deferredContext;
                commandList = other.commandList;
                commands = std::move(other.commands);
                thread = std::move(other.thread);
                working = other.working.load();

                other.deferredContext = nullptr;
                other.commandList = nullptr;
            }
            return *this;
        }
    };

	// �}���`�X���b�h�����_�����O�p�̃����o�[�ϐ�
	std::vector<ThreadData> m_threadDatas;
	bool m_useMultithreadedRendering;
	static const int THREAD_COUNT = 4; // �X���b�h��
    std::unique_ptr<DirectX::BasicEffect> m_basicEffect;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
public:
	PlayScene();
	~PlayScene() override;

	void Initialize(CommonResources* resources) override;
	void Update(float elapsedTime)override;
	void Render() override;
	void Finalize() override;

	SceneID GetNextSceneID() const;

    void RenderThread(int threadIndex);
};
