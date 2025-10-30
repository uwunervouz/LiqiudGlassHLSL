// Liquid Glass Vertex Shader (DirectX 11 / HLSL)

struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR;
    uint LiquidGlass : BLENDINDICES;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR;
    nointerpolation uint LiquidGlass : BLENDINDICES;
    nointerpolation float3 MidPoint : TEXCOORD1;
    nointerpolation float2 QuadScale : TEXCOORD2;
};

cbuffer TransformBuffer : register(b0)
{
    float4x4 ViewProjection;
    float3 ObjectPosition;
    float _pad0;
    float2 ObjectSize;
    float2 ScreenSize;
};

PSInput main(VSInput input)
{
    PSInput output;
    
    // Scale vertices by ObjectSize, then translate to ObjectPosition
    float3 scaledPos = input.Position * float3(ObjectSize.x, ObjectSize.y, 1.0);
    float4 worldPos = float4(scaledPos + ObjectPosition, 1.0);
    output.Position = mul(ViewProjection, worldPos);
    
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    output.LiquidGlass = input.LiquidGlass;
    
    // Calculate midpoint in NDC space
    float4 midPointWorld = float4(ObjectPosition, 1.0);
    float4 midPointNDC = mul(ViewProjection, midPointWorld);
    output.MidPoint = midPointNDC.xyz / midPointNDC.w;
    
    // Calculate quad scale for refraction
    // Need to project ObjectSize to screen space
    // Take a point offset by ObjectSize and see how it projects
    float4 offsetWorld = float4(ObjectPosition + float3(ObjectSize.x, ObjectSize.y, 0.0), 1.0);
    float4 offsetNDC = mul(ViewProjection, offsetWorld);
    float2 offsetProjected = offsetNDC.xy / offsetNDC.w;
    float2 midProjected = midPointNDC.xy / midPointNDC.w;
    output.QuadScale = abs(offsetProjected - midProjected);
    
    return output;
}
