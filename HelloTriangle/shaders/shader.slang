struct VSInput {
    float3 inPos;
    float3 inColor;
    float2 inTexCoord;
};

struct PushData {
    float4x4 viewproj;
};

struct Instance {
    float4x4 model;
}

struct InstanceBlock {
    Instance instances[100];
};

[[vk::push_constant]] ConstantBuffer<PushData> pc;
[[vk::binding(0, 0)]] ConstantBuffer<InstanceBlock> instances;
[[vk::binding(1, 0)]] Sampler2D tex;


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

    float4x4 model = instances.instances[instanceID].model;

    float4 pos = float4(input.inPos, 1.0f);
    output.pos = mul(pc.viewproj, mul(model, pos));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    return tex.Sample(vertIn.fragTexCoord);
}