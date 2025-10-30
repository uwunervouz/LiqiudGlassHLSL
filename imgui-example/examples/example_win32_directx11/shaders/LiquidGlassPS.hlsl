// Liquid Glass Pixel Shader (DirectX 11 / HLSL)

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR;
    nointerpolation uint LiquidGlass : BLENDINDICES;
    nointerpolation float3 MidPoint : TEXCOORD1;
    nointerpolation float2 QuadScale : TEXCOORD2;
};

cbuffer ShaderParams : register(b0)
{
    float u_powerFactor;
    float u_a;
    float u_b;
    float u_c;
    float u_d;
    float u_fPower;
    float u_noise;
    float u_glowWeight;
    float u_glowBias;
    float u_glowEdge0;
    float u_glowEdge1;
    float _pad[1];
};

Texture2D BackgroundTexture : register(t0);
Texture2D BlurredTexture : register(t1);
SamplerState LinearSampler : register(s0);

static const float M_E = 2.718281828459045;
static const float EPSILON = 0.00001;

// Signed distance field for superellipse (squircle)
float sdSuperellipse(float2 p, float n, float r)
{
    float2 p_abs = abs(p);
    
    float numerator = pow(p_abs.x, n) + pow(p_abs.y, n) - pow(r, n);
    
    float den_x = pow(p_abs.x, 2.0 * n - 2.0);
    float den_y = pow(p_abs.y, 2.0 * n - 2.0);
    float denominator = n * sqrt(den_x + den_y) + EPSILON;
    
    return numerator / denominator;
}

// Refraction function
float refractionFunc(float x)
{
    return 1.0 - u_b * pow(u_c * M_E, -u_d * x - u_a);
}

// Pseudo-random noise
float rand(float2 co)
{
    return frac(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

// Glow effect
float Glow(float2 texCoord)
{
    return sin(atan2(texCoord.y * 2.0 - 1.0, texCoord.x * 2.0 - 1.0) - 0.5);
}

float4 LiquidGlassEffect(PSInput input)
{
    float2 center = float2(0.5, 0.5);
    float2 p = (input.TexCoord - center) * 2.0;
    float r = 1.0;
    float d = sdSuperellipse(p, u_powerFactor, r);
    
    // Discard pixels outside the shape
    if (d > 0.0)
        discard;
    
    float dist = -d;
    float2 sampleP = p * pow(refractionFunc(dist), u_fPower);
    
    // Flip refraction direction for DirectX coordinate system
    sampleP.y = -sampleP.y;
    
    // Transform to screen space for texture lookup
    float2 targetNDC = sampleP * input.QuadScale + input.MidPoint.xy;
    float2 coord = targetNDC * 0.5 + float2(0.5, 0.5);
    
    // Return magenta for out-of-bounds
    if (max(coord.x, coord.y) > 1.0 || min(coord.x, coord.y) < 0.0)
        return float4(1.0, 0.0, 1.0, 1.0);
    
    // Sample blurred texture with noise
    float4 noise = float4((rand(input.Position.xy * 0.001) - 0.5).xxx, 0.0);
    float4 color = BlurredTexture.Sample(LinearSampler, coord) + noise * u_noise;
    
    // Apply glow
    float glowValue = Glow(input.TexCoord) * u_glowWeight * smoothstep(u_glowEdge0, u_glowEdge1, dist) + 1.0 + u_glowBias;
    return color * float4(glowValue.xxx, 1.0);
}

float4 main(PSInput input) : SV_TARGET
{
    // Mode 1: Liquid Glass effect
    if (input.LiquidGlass == 1)
        return LiquidGlassEffect(input);
    
    // Mode 2: Direct background rendering
    if (input.LiquidGlass == 2)
    {
        float2 coord = input.TexCoord;
        coord.y = 1.0 - coord.y;
        return BlurredTexture.Sample(LinearSampler, coord);
    }
    
    // Mode 0: Normal rendering
    return input.Color * BackgroundTexture.Sample(LinearSampler, input.TexCoord);
}
