struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

struct InstanceData {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct InstanceDataBuffer {
    InstanceData instances[4];
};

[[vk::binding(0,0)]]
ConstantBuffer<InstanceDataBuffer> idb;

Sampler2D texture;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input, uint instanceID: SV_InstanceID) {
    VSOutput output;

    InstanceData idata = idb.instances[instanceID];

    float4 pos = float4(input.inPos, 1.0f);
    output.pos = mul(idata.proj, mul(idata.view, mul(idata.model, pos)));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
   return texture.Sample(vertIn.fragTexCoord);
}