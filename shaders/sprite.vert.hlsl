struct VSInput {
    float2 position : TEXCOORD0;
    float2 texcoord : TEXCOORD1;
    float4 color : TEXCOORD2;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color : TEXCOORD1;
};

struct CameraData{
    float4x4 projection;
};

ConstantBuffer<CameraData> camera : register(b0, space1);

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = mul(camera.projection, float4(input.position, 0.0, 1.0));
    output.texcoord = input.texcoord;
    output.color = input.color;
    return output;
}
