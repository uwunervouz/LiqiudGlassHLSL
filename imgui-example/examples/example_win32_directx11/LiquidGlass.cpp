#include "LiquidGlass.h"
#include "imgui.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

// STB Image - define before including
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

LiquidGlass::LiquidGlass()
{
    // Initialize all pointers to nullptr
    m_device = nullptr;
    m_context = nullptr;
    m_liquidGlassVS = nullptr;
    m_liquidGlassPS = nullptr;
    m_blurVS = nullptr;
    m_blurPS = nullptr;
    m_simpleTexturePS = nullptr;
    m_inputLayout = nullptr;
    m_blurInputLayout = nullptr;
    m_vertexBuffer = nullptr;
    m_indexBuffer = nullptr;
    m_transformBuffer = nullptr;
    m_shaderParamsBuffer = nullptr;
    m_blurParamsBuffer = nullptr;
    m_backgroundRT = nullptr;
    m_backgroundRTV = nullptr;
    m_backgroundSRV = nullptr;
    m_blurIntermediateRT = nullptr;
    m_blurIntermediateRTV = nullptr;
    m_blurIntermediateSRV = nullptr;
    m_blurFinalRT = nullptr;
    m_blurFinalRTV = nullptr;
    m_blurFinalSRV = nullptr;
    m_linearSampler = nullptr;
    m_rasterizerState = nullptr;
    m_blendState = nullptr;
    m_depthStencilState = nullptr;
    m_currentBackgroundId = 0;
    m_position = XMFLOAT3(0.0f, 0.0f, 0.0f);  // Center of screen
    m_cameraPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);  // No camera offset
    m_velocityMultiplier = 1.0f;
    m_cameraVelocityMultiplier = 1.0f;
    m_velocity = 2.0f;
    m_cameraVelocity = 2.0f;
    m_width = 0.6f;
    m_height = 0.6f;
    m_blurIterations = 1;
    m_blurDownscaleFactor = 0.5f;
    m_mouseControl = false;
    m_screenWidth = 1280;
    m_screenHeight = 800;

    // Initialize shader parameters with default values
    m_shaderParams.u_powerFactor = 2.0f;
    m_shaderParams.u_a = 0.450f;
    m_shaderParams.u_b = 2.300f;
    m_shaderParams.u_c = 3.500f;
    m_shaderParams.u_d = 3.300f;
    m_shaderParams.u_fPower = 1.700f;
    m_shaderParams.u_noise = 0.000f;
    m_shaderParams.u_glowWeight = 0.320f;
    m_shaderParams.u_glowBias = 0.035f;
    m_shaderParams.u_glowEdge0 = 0.200f;
    m_shaderParams.u_glowEdge1 = -0.100f;

    m_blurParams.u_radius = 0.0f;
}

LiquidGlass::~LiquidGlass()
{
    Cleanup();
}

bool LiquidGlass::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int screenWidth, int screenHeight)
{
    m_device = device;
    m_context = context;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    if (!CreateShaders())
    {
        MessageBoxW(nullptr, L"Failed to create shaders! Check shaders/ folder.", L"LiquidGlass Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!CreateBuffers())
    {
        MessageBoxW(nullptr, L"Failed to create buffers!", L"LiquidGlass Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!CreateRenderTargets(screenWidth, screenHeight))
    {
        MessageBoxW(nullptr, L"Failed to create render targets!", L"LiquidGlass Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // Create sampler
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    m_device->CreateSamplerState(&samplerDesc, &m_linearSampler);

    // Create rasterizer state
    D3D11_RASTERIZER_DESC rastDesc = {};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE;  // Disable culling for debugging
    m_device->CreateRasterizerState(&rastDesc, &m_rasterizerState);

    // Create blend state
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_device->CreateBlendState(&blendDesc, &m_blendState);

    // Create depth stencil state with depth testing disabled
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = FALSE;  // Disable depth testing
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    m_device->CreateDepthStencilState(&depthDesc, &m_depthStencilState);

    // Load background texture
    Background bg;
    bg.name = "Default Background";
    bg.credits = "";
    if (LoadTexture("pic.jpg", &bg.texture, &bg.width, &bg.height))
    {
        m_backgrounds.push_back(bg);
        m_currentBackgroundId = 0;
    }
    else
    {
        MessageBoxW(nullptr, L"Warning: Could not jpg. Using solid color background.", L"Texture Load Warning", MB_OK | MB_ICONWARNING);
        m_currentBackgroundId = -1;
    }

    return true;
}

void LiquidGlass::Cleanup()
{
    // Release all COM objects
    if (m_liquidGlassVS) m_liquidGlassVS->Release();
    if (m_liquidGlassPS) m_liquidGlassPS->Release();
    if (m_blurVS) m_blurVS->Release();
    if (m_blurPS) m_blurPS->Release();
    if (m_simpleTexturePS) m_simpleTexturePS->Release();
    if (m_inputLayout) m_inputLayout->Release();
    if (m_blurInputLayout) m_blurInputLayout->Release();
    if (m_vertexBuffer) m_vertexBuffer->Release();
    if (m_indexBuffer) m_indexBuffer->Release();
    if (m_transformBuffer) m_transformBuffer->Release();
    if (m_shaderParamsBuffer) m_shaderParamsBuffer->Release();
    if (m_blurParamsBuffer) m_blurParamsBuffer->Release();
    if (m_backgroundRT) m_backgroundRT->Release();
    if (m_backgroundRTV) m_backgroundRTV->Release();
    if (m_backgroundSRV) m_backgroundSRV->Release();
    if (m_blurIntermediateRT) m_blurIntermediateRT->Release();
    if (m_blurIntermediateRTV) m_blurIntermediateRTV->Release();
    if (m_blurIntermediateSRV) m_blurIntermediateSRV->Release();
    if (m_blurFinalRT) m_blurFinalRT->Release();
    if (m_blurFinalRTV) m_blurFinalRTV->Release();
    if (m_blurFinalSRV) m_blurFinalSRV->Release();
    if (m_linearSampler) m_linearSampler->Release();
    if (m_rasterizerState) m_rasterizerState->Release();
    if (m_blendState) m_blendState->Release();
    if (m_depthStencilState) m_depthStencilState->Release();

    for (auto& bg : m_backgrounds)
    {
        if (bg.texture) bg.texture->Release();
    }
}

void LiquidGlass::Update(float deltaTime)
{
    ImGuiIO& io = ImGui::GetIO();

    // Handle object movement (WASD keys)
    if (!m_mouseControl)
    {
        bool anyKeyPressed = false;
        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            m_position.y += deltaTime * m_velocityMultiplier * m_velocity;
            anyKeyPressed = true;
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            m_position.y -= deltaTime * m_velocityMultiplier * m_velocity;
            anyKeyPressed = true;
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            m_position.x += deltaTime * m_velocityMultiplier * m_velocity;
            anyKeyPressed = true;
        }
        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            m_position.x -= deltaTime * m_velocityMultiplier * m_velocity;
            anyKeyPressed = true;
        }

        m_velocityMultiplier += (anyKeyPressed ? 1.0f : -3.0f) * deltaTime;
        m_velocityMultiplier = max(0.0f, min(1.0f, m_velocityMultiplier));
    }

    // Handle camera movement (Arrow keys)
    bool cameraKeyPressed = false;
    if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
        m_cameraPosition.y -= deltaTime * m_cameraVelocityMultiplier * m_cameraVelocity;
        cameraKeyPressed = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
        m_cameraPosition.y += deltaTime * m_cameraVelocityMultiplier * m_cameraVelocity;
        cameraKeyPressed = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
        m_cameraPosition.x -= deltaTime * m_cameraVelocityMultiplier * m_cameraVelocity;
        cameraKeyPressed = true;
    }
    if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
        m_cameraPosition.x += deltaTime * m_cameraVelocityMultiplier * m_cameraVelocity;
        cameraKeyPressed = true;
    }

    m_cameraVelocityMultiplier += (cameraKeyPressed ? 1.0f : -3.0f) * deltaTime;
    m_cameraVelocityMultiplier = max(0.0f, min(1.0f, m_cameraVelocityMultiplier));

    // Mouse control
    if (m_mouseControl)
    {
        float mouseX = (io.MousePos.x / m_screenWidth - 0.5f) * 15.0f;
        float mouseY = -(io.MousePos.y / m_screenHeight - 0.5f) * 15.0f * m_screenHeight / m_screenWidth;
        m_position.x = mouseX;
        m_position.y = mouseY;
    }
}

void LiquidGlass::OnResize(int width, int height)
{
    m_screenWidth = width;
    m_screenHeight = height;

    // Release old render targets
    if (m_backgroundRT) m_backgroundRT->Release();
    if (m_backgroundRTV) m_backgroundRTV->Release();
    if (m_backgroundSRV) m_backgroundSRV->Release();
    if (m_blurIntermediateRT) m_blurIntermediateRT->Release();
    if (m_blurIntermediateRTV) m_blurIntermediateRTV->Release();
    if (m_blurIntermediateSRV) m_blurIntermediateSRV->Release();
    if (m_blurFinalRT) m_blurFinalRT->Release();
    if (m_blurFinalRTV) m_blurFinalRTV->Release();
    if (m_blurFinalSRV) m_blurFinalSRV->Release();

    // Recreate render targets
    CreateRenderTargets(width, height);
}

void LiquidGlass::RenderUI()
{
    // Set initial window position and size (ALWAYS to ensure visibility)
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
    
    // Begin window without close button to ensure it's always visible
    if (!ImGui::Begin("Liquid Glass Settings", nullptr, ImGuiWindowFlags_None))
    {
        // Window is collapsed, but still need to end it
        ImGui::End();
        return;
    }

    // ПОКАЗЫВАЕМ КАРТИНКУ ЧЕРЕЗ IMGUI::IMAGE!!!
    ImGui::Text("LOADED IMAGE:");
    if (m_currentBackgroundId >= 0 && m_currentBackgroundId < (int)m_backgrounds.size())
    {
        Background& bg = m_backgrounds[m_currentBackgroundId];
        ImGui::Image((void*)bg.texture, ImVec2(512, 288));
        ImGui::Text("Size: %dx%d", bg.width, bg.height);
    }
    else
    {
        ImGui::Text("NO IMAGE LOADED!");
    }
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move glass object");
    ImGui::BulletText("Arrow Keys - Move camera");
    ImGui::BulletText("Mouse Scroll - Zoom");
    
    ImGui::Separator();
    ImGui::Checkbox("Move with mouse", &m_mouseControl);

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Shape", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Power", &m_shaderParams.u_powerFactor, 1.001f, 6.0f);
        ImGui::SliderFloat("Width", &m_width, 0.0f, 10.0f);
        ImGui::SliderFloat("Height", &m_height, 0.0f, 10.0f);
    }

    if (ImGui::CollapsingHeader("Blur & Noise", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderInt("Blur Iterations", &m_blurIterations, 0, 10);
        ImGui::SliderFloat("Blur Radius", &m_blurParams.u_radius, 0.0f, 10.0f);
        ImGui::SliderFloat("Blur Downscale", &m_blurDownscaleFactor, 0.1f, 1.0f);
        ImGui::SliderFloat("Noise", &m_shaderParams.u_noise, 0.0f, 0.3f);
    }

    if (ImGui::CollapsingHeader("Refraction", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("f(x) = 1 - b * (c*e)^(-d*x-a)");
        ImGui::SliderFloat("f(x) Power", &m_shaderParams.u_fPower, -1.5f, 6.0f);
        ImGui::SliderFloat("a", &m_shaderParams.u_a, 0.0f, 5.0f);
        ImGui::SliderFloat("b", &m_shaderParams.u_b, 0.0f, 6.0f);
        ImGui::SliderFloat("c", &m_shaderParams.u_c, 0.0f, 6.0f);
        ImGui::SliderFloat("d", &m_shaderParams.u_d, 0.0f, 10.0f);
    }

    if (ImGui::CollapsingHeader("Glow", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("Glow Weight", &m_shaderParams.u_glowWeight, -1.0f, 1.0f);
        ImGui::SliderFloat("Glow Bias", &m_shaderParams.u_glowBias, -1.0f, 1.0f);
        ImGui::SliderFloat("Glow Edge0", &m_shaderParams.u_glowEdge0, -1.0f, 1.0f);
        ImGui::SliderFloat("Glow Edge1", &m_shaderParams.u_glowEdge1, -1.0f, 1.0f);
    }
    
    // DEBUG: Show textures
    if (ImGui::CollapsingHeader("Debug Textures", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Background Texture:");
        if (m_backgroundSRV)
            ImGui::Image((void*)m_backgroundSRV, ImVec2(256, 144));
        
        ImGui::Text("Blurred Texture:");
        if (m_blurFinalSRV)
            ImGui::Image((void*)m_blurFinalSRV, ImVec2(256, 144));
        
        ImGui::Text("Source Image:");
        if (m_currentBackgroundId >= 0 && m_currentBackgroundId < (int)m_backgrounds.size())
        {
            Background& bg = m_backgrounds[m_currentBackgroundId];
            ImGui::Image((void*)bg.texture, ImVec2(256, 144));
            ImGui::Text("Size: %dx%d", bg.width, bg.height);
        }
    }

    ImGui::End();
}
bool LiquidGlass::CreateShaders()
{
    HRESULT hr;
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // Compile Liquid Glass Vertex Shader
    hr = D3DCompileFromFile(L"shaders/LiquidGlassVS.hlsl", nullptr, nullptr, "main", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        return false;
    }

    m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_liquidGlassVS);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    m_device->CreateInputLayout(layout, 4, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
    vsBlob->Release();

    // Compile Liquid Glass Pixel Shader
    hr = D3DCompileFromFile(L"shaders/LiquidGlassPS.hlsl", nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        return false;
    }
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_liquidGlassPS);
    psBlob->Release();

    // Compile Blur Shaders
    hr = D3DCompileFromFile(L"shaders/BlurVS.hlsl", nullptr, nullptr, "main", "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        return false;
    }
    m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_blurVS);

    D3D11_INPUT_ELEMENT_DESC blurLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    m_device->CreateInputLayout(blurLayout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_blurInputLayout);
    vsBlob->Release();

    hr = D3DCompileFromFile(L"shaders/BlurPS.hlsl", nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        return false;
    }
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_blurPS);
    psBlob->Release();

    // Compile Simple Texture Shader for background rendering
    hr = D3DCompileFromFile(L"shaders/SimpleTexturePS.hlsl", nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob) { OutputDebugStringA((char*)errorBlob->GetBufferPointer()); errorBlob->Release(); }
        return false;
    }
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_simpleTexturePS);
    psBlob->Release();

    return true;
}

bool LiquidGlass::CreateBuffers()
{
    // Create quad vertices
    Vertex quadVertices[] = {
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1 },
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1 },
        { XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1 },
        { XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1 }
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = sizeof(quadVertices);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = quadVertices;
    m_device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer);

    UINT indices[] = { 0, 1, 2, 2, 3, 0 };
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;
    m_device->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer);

    // Constant buffers
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    cbDesc.ByteWidth = sizeof(TransformBuffer);
    m_device->CreateBuffer(&cbDesc, nullptr, &m_transformBuffer);

    cbDesc.ByteWidth = sizeof(ShaderParams);
    m_device->CreateBuffer(&cbDesc, nullptr, &m_shaderParamsBuffer);

    cbDesc.ByteWidth = sizeof(BlurParams);
    m_device->CreateBuffer(&cbDesc, nullptr, &m_blurParamsBuffer);

    return true;
}

bool LiquidGlass::CreateRenderTargets(int width, int height)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    m_device->CreateTexture2D(&texDesc, nullptr, &m_backgroundRT);
    m_device->CreateRenderTargetView(m_backgroundRT, nullptr, &m_backgroundRTV);
    m_device->CreateShaderResourceView(m_backgroundRT, nullptr, &m_backgroundSRV);

    int blurWidth = (int)(width * m_blurDownscaleFactor);
    int blurHeight = (int)(height * m_blurDownscaleFactor);
    texDesc.Width = blurWidth;
    texDesc.Height = blurHeight;

    m_device->CreateTexture2D(&texDesc, nullptr, &m_blurIntermediateRT);
    m_device->CreateRenderTargetView(m_blurIntermediateRT, nullptr, &m_blurIntermediateRTV);
    m_device->CreateShaderResourceView(m_blurIntermediateRT, nullptr, &m_blurIntermediateSRV);

    m_device->CreateTexture2D(&texDesc, nullptr, &m_blurFinalRT);
    m_device->CreateRenderTargetView(m_blurFinalRT, nullptr, &m_blurFinalRTV);
    m_device->CreateShaderResourceView(m_blurFinalRT, nullptr, &m_blurFinalSRV);

    return true;
}

bool LiquidGlass::LoadTexture(const char* filename, ID3D11ShaderResourceView** textureView, int* width, int* height)
{
    int channels;
    unsigned char* data = stbi_load(filename, width, height, &channels, 4);
    if (!data) return false;

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = *width;
    texDesc.Height = *height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    initData.SysMemPitch = *width * 4;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = m_device->CreateTexture2D(&texDesc, &initData, &texture);
    stbi_image_free(data);

    if (FAILED(hr)) return false;

    hr = m_device->CreateShaderResourceView(texture, nullptr, textureView);
    texture->Release();

    return SUCCEEDED(hr);
}

void LiquidGlass::UpdateConstantBuffers()
{
    D3D11_MAPPED_SUBRESOURCE mapped;

    // Transform buffer
    m_context->Map(m_transformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    TransformBuffer* transformData = (TransformBuffer*)mapped.pData;

    float orthoSize = 15.0f;
    float aspect = (float)m_screenHeight / (float)m_screenWidth;
    XMMATRIX projection = XMMatrixOrthographicLH(orthoSize, orthoSize * aspect, -10.0f, 10.0f);
    XMMATRIX view = XMMatrixTranslation(-m_cameraPosition.x, -m_cameraPosition.y, -m_cameraPosition.z);

    transformData->ViewProjection = XMMatrixTranspose(projection * view);
    transformData->ObjectPosition = m_position;
    transformData->ObjectSize = XMFLOAT2(m_width, m_height);
    transformData->ScreenSize = XMFLOAT2((float)m_screenWidth, (float)m_screenHeight);
    m_context->Unmap(m_transformBuffer, 0);

    // Shader params
    m_context->Map(m_shaderParamsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &m_shaderParams, sizeof(ShaderParams));
    m_context->Unmap(m_shaderParamsBuffer, 0);
}

void LiquidGlass::RenderBackground()
{
    // Set render target
    m_context->OMSetRenderTargets(1, &m_backgroundRTV, nullptr);
    
    // Clear background
    float clearColor[4] = { 0.2f, 0.2f, 0.3f, 1.0f };  // Dark blue fallback
    m_context->ClearRenderTargetView(m_backgroundRTV, clearColor);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)m_screenWidth;
    viewport.Height = (float)m_screenHeight;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);
    
    // If we have a background texture, render it
    if (m_currentBackgroundId >= 0 && m_currentBackgroundId < (int)m_backgrounds.size())
    {
        Background& bg = m_backgrounds[m_currentBackgroundId];
        
        // Use simple texture shader to draw fullscreen textured quad
        m_context->IASetInputLayout(m_blurInputLayout);
        m_context->VSSetShader(m_blurVS, nullptr, 0);
        m_context->PSSetShader(m_simpleTexturePS, nullptr, 0);  // Use simple texture shader
        m_context->PSSetShaderResources(0, 1, &bg.texture);
        m_context->PSSetSamplers(0, 1, &m_linearSampler);
        
        // Draw using main vertex buffer (it's a quad)
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        m_context->DrawIndexed(6, 0, 0);
        
        // Unbind
        ID3D11ShaderResourceView* nullSRV = nullptr;
        m_context->PSSetShaderResources(0, 1, &nullSRV);
    }
}

void LiquidGlass::ApplyBlur()
{
    if (m_blurIterations == 0) return;

    m_context->IASetInputLayout(m_blurInputLayout);
    m_context->VSSetShader(m_blurVS, nullptr, 0);
    m_context->PSSetShader(m_blurPS, nullptr, 0);
    m_context->PSSetSamplers(0, 1, &m_linearSampler);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = m_screenWidth * m_blurDownscaleFactor;
    viewport.Height = m_screenHeight * m_blurDownscaleFactor;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (int i = 0; i < m_blurIterations; i++)
    {
        ID3D11ShaderResourceView* inputSRV = (i == 0) ? m_backgroundSRV : m_blurFinalSRV;

        // Horizontal
        D3D11_MAPPED_SUBRESOURCE mapped;
        m_context->Map(m_blurParamsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        BlurParams* blur = (BlurParams*)mapped.pData;
        blur->u_direction = XMFLOAT2(1.0f, 0.0f);
        blur->u_resolution = XMFLOAT2(viewport.Width, viewport.Height);
        blur->u_radius = m_blurParams.u_radius;
        m_context->Unmap(m_blurParamsBuffer, 0);
        m_context->PSSetConstantBuffers(0, 1, &m_blurParamsBuffer);

        m_context->OMSetRenderTargets(1, &m_blurIntermediateRTV, nullptr);
        m_context->PSSetShaderResources(0, 1, &inputSRV);
        m_context->DrawIndexed(6, 0, 0);

        // Vertical
        m_context->Map(m_blurParamsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        blur = (BlurParams*)mapped.pData;
        blur->u_direction = XMFLOAT2(0.0f, 1.0f);
        blur->u_resolution = XMFLOAT2(viewport.Width, viewport.Height);
        blur->u_radius = m_blurParams.u_radius;
        m_context->Unmap(m_blurParamsBuffer, 0);

        m_context->OMSetRenderTargets(1, &m_blurFinalRTV, nullptr);
        m_context->PSSetShaderResources(0, 1, &m_blurIntermediateSRV);
        m_context->DrawIndexed(6, 0, 0);
    }

    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_context->PSSetShaderResources(0, 1, &nullSRV);
}

void LiquidGlass::RenderLiquidGlass()
{
    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (float)m_screenWidth;
    viewport.Height = (float)m_screenHeight;
    viewport.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &viewport);
    
    // Set blend state for transparent rendering
    m_context->OMSetBlendState(m_blendState, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_depthStencilState, 0);
    m_context->RSSetState(m_rasterizerState);
    
    m_context->IASetInputLayout(m_inputLayout);
    m_context->VSSetShader(m_liquidGlassVS, nullptr, 0);
    m_context->PSSetShader(m_liquidGlassPS, nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_transformBuffer);
    m_context->PSSetConstantBuffers(0, 1, &m_shaderParamsBuffer);
    m_context->PSSetShaderResources(0, 1, &m_backgroundSRV);
    m_context->PSSetShaderResources(1, 1, &m_blurFinalSRV);
    m_context->PSSetSamplers(0, 1, &m_linearSampler);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_context->DrawIndexed(6, 0, 0);

    ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
    m_context->PSSetShaderResources(0, 2, nullSRVs);
}

void LiquidGlass::Render(ID3D11RenderTargetView* mainRenderTarget)
{
    UpdateConstantBuffers();
    RenderBackground();  // Render to internal RT for blur reference
    ApplyBlur();         // Blur the background
    
    // Set main render target for final render
    m_context->OMSetRenderTargets(1, &mainRenderTarget, nullptr);
    
    // Draw liquid glass effect (background is already drawn by ImGui)
    RenderLiquidGlass();
}
