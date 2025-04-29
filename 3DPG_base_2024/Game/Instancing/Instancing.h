#pragma once

#include"Game/ShaderManager.h"



class Instancing
{
private:
	static const int MAX_INSTANCE = 100000;

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

	int m_maxInstance;

	// ���f��
	std::unique_ptr<DirectX::Model> m_model;
	//�e�N�X�`��
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
	//�V�F�[�_�[
	ShaderSet m_instanceSet;
	//�萔�o�b�t�@
	Microsoft::WRL::ComPtr<ID3D11Buffer>	cBuffer;
public:
	Instancing();
	~Instancing() = default;

	void Initialize(ID3D11Device* device,int maxinstance = MAX_INSTANCE);
	void LoadModel(std::string filename,ID3D11Device* device);
	void LoadTexture(std::string filename, ID3D11Device* device);
	void Render(ID3D11DeviceContext* context,
		DirectX::DX11::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& projection,
		std::vector<DirectX::SimpleMath::Matrix>& worlds);

	void Render(ID3D11DeviceContext* context,
		DirectX::DX11::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& projection);
};