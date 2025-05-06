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

	std::vector<DirectX::SimpleMath::Matrix> m_worlds;

	//�}���`�X���b�h�֌W
	std::vector<ID3D11DeviceContext*> m_deferradContext;
	std::vector<ID3D11CommandList*> m_commnds;
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
};
