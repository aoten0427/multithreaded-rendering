/*
	@file	PlayScene.h
	@brief	�v���C�V�[���N���X
*/
#pragma once
#include "IScene.h"
#include <thread>
#include <mutex>
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
	static const int MAX_INSTANCE = 1;

	struct PNTStaticConstantBuffer
	{
		DirectX::SimpleMath::Matrix World;
		DirectX::SimpleMath::Matrix View;
		DirectX::SimpleMath::Matrix Projection;
		DirectX::SimpleMath::Vector4 LightDir;
		DirectX::SimpleMath::Vector4 Emissive;
		DirectX::SimpleMath::Vector4 Diffuse;
		PNTStaticConstantBuffer() {
			memset(this, 0, sizeof(PNTStaticConstantBuffer));
		};
	};

	struct CBuff
	{
		DirectX::SimpleMath::Matrix mat[MAX_INSTANCE];
	};

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

	ShaderSet m_instanceSet;

	//�萔�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D11Buffer>	cBuffer;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_tex;
public:
	PlayScene();
	~PlayScene() override;

	void Initialize(CommonResources* resources) override;
	void Update(float elapsedTime)override;
	void Render() override;
	void Finalize() override;

	SceneID GetNextSceneID() const;
};
