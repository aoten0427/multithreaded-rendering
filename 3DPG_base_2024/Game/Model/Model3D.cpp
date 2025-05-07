#include"pch.h"
#include"Model3D.h"
#include"DeviceResources.h"

Model3D::Model3D()
	:
	m_model{}
{

}

void Model3D::Initialize(ID3D11Device* device)
{


	
}

void Model3D::LoadModel(DirectX::Model* model)
{
	m_model = model;
}

void Model3D::LoadTexture(ID3D11ShaderResourceView* texture)
{
	m_texture = texture;
}



void Model3D::Render(ID3D11DeviceContext* context, DirectX::DX11::CommonStates* states, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& projection)
{
	for (auto& meshes : m_model->meshes)
	{
		for (auto& mesh : meshes->meshParts)
		{
			UINT stride = sizeof(DirectX::VertexPositionNormalTangentColorTexture); // VertexType�͎��ۂ̒��_�\���̂ɍ��킹��
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, mesh->vertexBuffer.GetAddressOf(), &stride, &offset);
			//�C���f�b�N�X�o�b�t�@�̃Z�b�g
			context->IASetIndexBuffer(mesh->indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			//�`����@�i3�p�`�jD3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			//�V�F�[�_�̐ݒ�
			context->VSSetShader(m_shaderSet.vertexShader.Get(), nullptr, 0);
			context->PSSetShader(m_shaderSet.pixelShader.Get(), nullptr, 0);
			//�C���v�b�g���C�A�E�g�̐ݒ�
			context->IASetInputLayout(m_shaderSet.inputLayout.Get());

			//�u�����h�X�e�[�g
		    //�����������Ȃ�
			context->OMSetBlendState(states->Opaque(), nullptr, 0xffffffff);
			//�f�v�X�X�e���V���X�e�[�g
			context->OMSetDepthStencilState(states->DepthDefault(), 0);
			//�e�N�X�`���ƃT���v���[�̐ݒ�
			ID3D11ShaderResourceView* pNull[1] = { 0 };
			context->PSSetShaderResources(0, 1,&m_texture);
			ID3D11SamplerState* pSampler = states->LinearWrap();;
			context->PSSetSamplers(0, 1, &pSampler);
			//���X�^���C�U�X�e�[�g�i�\�ʕ`��j
			context->RSSetState(states->CullNone());

			CBuff cb;
			DirectX::SimpleMath::Matrix world = m_matrix * view * projection;
			cb.mat = world.Transpose();

			//�R���X�^���g�o�b�t�@�̍X�V
			context->UpdateSubresource(m_shaderSet.cBuffer.Get(), 0, nullptr, &cb, 0, 0);
			//�R���X�^���g�o�b�t�@�̐ݒ�
			ID3D11Buffer* pConstantBuffer = m_shaderSet.cBuffer.Get();
			ID3D11Buffer* pNullConstantBuffer = nullptr;
			//���_�V�F�[�_�ɓn��
			context->VSSetConstantBuffers(0, 1, &pConstantBuffer);
			//�s�N�Z���V�F�[�_�ɓn��
			context->PSSetConstantBuffers(0, 1, &pConstantBuffer);
			//�`��
			// �C���f�b�N�X�o�b�t�@���g�p�����`��ɕύX
			context->DrawIndexed(mesh->indexCount, 0, 0);  // Draw�̑����DrawIndexed���g�p
		}
	}

}

void Model3D::SetShader(ShaderSet& set)
{
	m_shaderSet.vertexShader = set.vertexShader;
	m_shaderSet.pixelShader = set.pixelShader;
	m_shaderSet.inputLayout = set.inputLayout;
	m_shaderSet.cBuffer = set.cBuffer;
}
