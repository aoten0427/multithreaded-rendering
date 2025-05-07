#include"Model.hlsli"

PS_INPUT main(VS_INPUT input)
{
    
    PS_INPUT output = (PS_INPUT) 0;
    //�e�N�X�`�����Z�b�g
    output.tex = input.Tex;
    //�@�����Z�b�g
    output.norm = input.Normal;
   
    output.position = mul(float4(input.Pos, 1), WorldViewProj);
    
    return output;
}