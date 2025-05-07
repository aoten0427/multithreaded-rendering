#pragma once
#include"Game/ShaderManager.h"

class Model3D
{
private:
	struct CBuff
	{
		DirectX::SimpleMath::Matrix mat;
	};
private:
	DirectX::Model* m_model;
	//テクスチャ
	ID3D11ShaderResourceView* m_texture;

	ShaderSet m_shaderSet;

	DirectX::SimpleMath::Matrix m_matrix;
public:
	Model3D();
	~Model3D() = default;

	void Initialize(ID3D11Device* device);
	void LoadModel(DirectX::Model* model);
	void LoadTexture(ID3D11ShaderResourceView* texture);
	void Render(ID3D11DeviceContext* context,
		DirectX::DX11::CommonStates* states,
		const DirectX::SimpleMath::Matrix& view,
		const DirectX::SimpleMath::Matrix& projection);

	void SetMatrix(DirectX::SimpleMath::Matrix& mat) { m_matrix = mat; }

	void SetShader(ShaderSet& set);
};