// Blur Pixel Shader (DirectX 11 / HLSL)
// 13-tap Gaussian blur

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

cbuffer BlurParams : register(b0)
{
    float2 u_direction;
    float2 u_resolution;
    float u_radius;
    float3 _pad;
};

Texture2D InputTexture : register(t0);
SamplerState LinearSampler : register(s0);

float4 blur13(Texture2D image, float2 uv, float2 resolution, float2 direction)
{
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    float2 off1 = float2(1.411764705882353, 1.411764705882353) * direction;
    float2 off2 = float2(3.2941176470588234, 3.2941176470588234) * direction;
    float2 off3 = float2(5.176470588235294, 5.176470588235294) * direction;
    
    color += image.Sample(LinearSampler, uv) * 0.1964825501511404;
    color += image.Sample(LinearSampler, uv + (off1 / resolution)) * 0.2969069646728344;
    color += image.Sample(LinearSampler, uv - (off1 / resolution)) * 0.2969069646728344;
    color += image.Sample(LinearSampler, uv + (off2 / resolution)) * 0.09447039785044732;
    color += image.Sample(LinearSampler, uv - (off2 / resolution)) * 0.09447039785044732;
    color += image.Sample(LinearSampler, uv + (off3 / resolution)) * 0.010381362401148057;
    color += image.Sample(LinearSampler, uv - (off3 / resolution)) * 0.010381362401148057;
    
    return color;
}

float4 main(PSInput input) : SV_TARGET
{
    // Flip Y coordinate for DirectX
    float2 uv = input.TexCoord;
    uv.y = 1.0 - uv.y;
    
    float4 result = blur13(InputTexture, uv, u_resolution, u_direction * u_radius);
    return result;
}
