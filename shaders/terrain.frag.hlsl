struct PSInput {
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float4 color : TEXCOORD1;
};

struct TerrainLightData {
    float4 directionAndAmbient;
};

ConstantBuffer<TerrainLightData> terrainLight : register(b0, space3);

float4 main(PSInput input) : SV_Target0 {
    const float3 normal = normalize(input.normal);
    const float3 lightDirection = normalize(terrainLight.directionAndAmbient.xyz);
    const float ambient = saturate(terrainLight.directionAndAmbient.w);
    const float diffuse = saturate(dot(normal, lightDirection));
    const float lighting = ambient + diffuse * (1.0 - ambient);
    return float4(input.color.rgb * lighting, input.color.a);
}
