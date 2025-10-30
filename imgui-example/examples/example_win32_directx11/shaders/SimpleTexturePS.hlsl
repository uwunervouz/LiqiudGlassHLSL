// Simple Texture Pixel Shader (DirectX 11 / HLSL)
// Just displays a texture on a quad

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

Texture2D InputTexture : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    // Flip Y coordinate for DirectX texture coordinates
    float2 uv = input.TexCoord;
    uv.y = 1.0 - uv.y;
    
    return InputTexture.Sample(LinearSampler, uv);
}
