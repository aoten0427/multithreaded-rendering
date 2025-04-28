#include"InstanceModel.hlsli"

cbuffer SimpleConstantBuffer : register(b0)
{
    float4x4 World : packoffset(c0); //���̃��[���h�s��͎g�p���Ȃ�
    float4x4 View : packoffset(c4);
    float4x4 Projection : packoffset(c8);
    float4 LightDir : packoffset(c12);
    float4 Emissive : packoffset(c13);
    float4 Diffuse : packoffset(c14);
};



// ���_�V�F�[�_�[�̏C��
PSPNTInput main(VSPNTInstanceInput input)
{
    PSPNTInput result;
    
    // ���_�̈ʒu��ϊ�
    float4 pos = float4(input.position.xyz, 1.0f);
    
    // ���[���h�ϊ��i�C���X�^���X�s���K�p�j
    pos = mul(pos, input.mat);
    
    // �r���[�ϊ�
    pos = mul(pos, View);
    
    // �ˉe�ϊ�
    pos = mul(pos, Projection);
    
    // �o�͈ʒu�̐ݒ�
    result.position = pos;
    
    // �@���̕ϊ��i�C���X�^���X�s��ŕϊ��j
    // �����ŋt�s��̓]�u���g�p����Ƃ��悢���ʂ�������ꍇ������
    result.norm = mul(input.norm, (float3x3) input.mat);
    result.norm = normalize(result.norm);
    
    // �e�N�X�`��UV
    result.tex = input.tex;
    
    return result;
}