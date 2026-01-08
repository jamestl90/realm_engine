struct PSInput {
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color : TEXCOORD1;
};

Texture2D spriteTexture : register(t0, space2);
SamplerState spriteSampler : register(s0, space2);

float4 main(PSInput input) : SV_Target0 {
    float4 texColor = spriteTexture.Sample(spriteSampler, input.texcoord);
    return texColor * input.color;
}
