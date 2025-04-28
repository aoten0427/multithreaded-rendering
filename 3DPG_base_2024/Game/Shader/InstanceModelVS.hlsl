#include"InstanceModel.hlsli"

cbuffer SimpleConstantBuffer : register(b0)
{
    float4x4 World : packoffset(c0); //このワールド行列は使用しない
    float4x4 View : packoffset(c4);
    float4x4 Projection : packoffset(c8);
    float4 LightDir : packoffset(c12);
    float4 Emissive : packoffset(c13);
    float4 Diffuse : packoffset(c14);
};



// 頂点シェーダーの修正
PSPNTInput main(VSPNTInstanceInput input)
{
    PSPNTInput result;
    
    // 頂点の位置を変換
    float4 pos = float4(input.position.xyz, 1.0f);
    
    // ワールド変換（インスタンス行列を適用）
    pos = mul(pos, input.mat);
    
    // ビュー変換
    pos = mul(pos, View);
    
    // 射影変換
    pos = mul(pos, Projection);
    
    // 出力位置の設定
    result.position = pos;
    
    // 法線の変換（インスタンス行列で変換）
    // ここで逆行列の転置を使用するとよりよい結果が得られる場合もある
    result.norm = mul(input.norm, (float3x3) input.mat);
    result.norm = normalize(result.norm);
    
    // テクスチャUV
    result.tex = input.tex;
    
    return result;
}