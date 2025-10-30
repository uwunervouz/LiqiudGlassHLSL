#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>

using namespace DirectX;

struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT2 TexCoord;
    XMFLOAT4 Color;
    UINT LiquidGlass;
};

struct TransformBuffer
{
    XMMATRIX ViewProjection;
    XMFLOAT3 ObjectPosition;
    float _pad0;
    XMFLOAT2 ObjectSize;
    XMFLOAT2 ScreenSize;
};

struct ShaderParams
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

struct BlurParams
{
    XMFLOAT2 u_direction;
    XMFLOAT2 u_resolution;
    float u_radius;
    XMFLOAT3 _pad;
};

struct Background
{
    std::string name;
    std::string credits;
    ID3D11ShaderResourceView* texture;
    int width;
    int height;
};

class LiquidGlass
{
public:
    LiquidGlass();
    ~LiquidGlass();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int screenWidth, int screenHeight);
    void Cleanup();
    void Update(float deltaTime);
    void Render(ID3D11RenderTargetView* mainRenderTarget);
    void OnResize(int width, int height);
    void RenderUI();
    
    // Getter for backgrounds
    const std::vector<Background>& GetBackgrounds() const { return m_backgrounds; }

private:
    // Helper methods
    bool CreateShaders();
    bool CreateBuffers();
    bool CreateRenderTargets(int width, int height);
    bool LoadTexture(const char* filename, ID3D11ShaderResourceView** textureView, int* width, int* height);
    void UpdateConstantBuffers();
    void RenderBackground();
    void ApplyBlur();
    void RenderLiquidGlass();

private:
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;

    // Shaders
    ID3D11VertexShader* m_liquidGlassVS;
    ID3D11PixelShader* m_liquidGlassPS;
    ID3D11VertexShader* m_blurVS;
    ID3D11PixelShader* m_blurPS;
    ID3D11PixelShader* m_simpleTexturePS;  // For rendering background texture
    ID3D11InputLayout* m_inputLayout;
    ID3D11InputLayout* m_blurInputLayout;

    // Buffers
    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_indexBuffer;
    ID3D11Buffer* m_transformBuffer;
    ID3D11Buffer* m_shaderParamsBuffer;
    ID3D11Buffer* m_blurParamsBuffer;

    // Render targets
    ID3D11Texture2D* m_backgroundRT;
    ID3D11RenderTargetView* m_backgroundRTV;
    ID3D11ShaderResourceView* m_backgroundSRV;

    ID3D11Texture2D* m_blurIntermediateRT;
    ID3D11RenderTargetView* m_blurIntermediateRTV;
    ID3D11ShaderResourceView* m_blurIntermediateSRV;

    ID3D11Texture2D* m_blurFinalRT;
    ID3D11RenderTargetView* m_blurFinalRTV;
    ID3D11ShaderResourceView* m_blurFinalSRV;

    // Samplers
    ID3D11SamplerState* m_linearSampler;

    // Rasterizer states
    ID3D11RasterizerState* m_rasterizerState;
    ID3D11BlendState* m_blendState;
    ID3D11DepthStencilState* m_depthStencilState;

    // Backgrounds
    std::vector<Background> m_backgrounds;
    int m_currentBackgroundId;

    // Shader parameters
    ShaderParams m_shaderParams;
    BlurParams m_blurParams;
    TransformBuffer m_transformData;

    // Animation state
    XMFLOAT3 m_position;
    XMFLOAT3 m_cameraPosition;
    float m_velocityMultiplier;
    float m_cameraVelocityMultiplier;
    float m_velocity;
    float m_cameraVelocity;
    float m_width;
    float m_height;
    int m_blurIterations;
    float m_blurDownscaleFactor;
    bool m_mouseControl;

    int m_screenWidth;
    int m_screenHeight;
};
