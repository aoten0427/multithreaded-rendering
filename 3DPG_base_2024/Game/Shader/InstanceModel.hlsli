struct VSPNTInstanceInput
{
    float4 position : POSITION;
    float3 norm : NORMAL;
    float2 tex : TEXCOORD0;
    float4x4 mat : MATRIX;
    uint InstanceId : SV_InstanceID;
};

struct PSPNTInput
{
    float4 position : SV_POSITION;
    float3 norm : NORMAL;
    float2 tex : TEXCOORD;
};