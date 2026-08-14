struct VSInput {
    float3 position : TEXCOORD0;
    float2 gradient : TEXCOORD1;
    float4 color : TEXCOORD2;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float4 color : TEXCOORD1;
};

struct TerrainViewData {
    float4x4 viewProjection;
    float4 terrainParameters;
};

ConstantBuffer<TerrainViewData> terrainView : register(b0, space1);

VSOutput main(VSInput input) {
    VSOutput output;
    const float elevationScale = terrainView.terrainParameters.x;
    const float3 worldPosition = float3(
        input.position.x,
        input.position.y,
        input.position.z * elevationScale
    );

    output.position = mul(terrainView.viewProjection, float4(worldPosition, 1.0));
    output.normal = normalize(float3(
        -input.gradient.x * elevationScale,
        -input.gradient.y * elevationScale,
        1.0
    ));
    output.color = input.color;
    return output;
}
