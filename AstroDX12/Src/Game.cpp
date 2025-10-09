//
// Game.cpp
//

#include <Game.h>
#include <Maths/MathUtils.h>
#include <Rendering/RendererDX12.h>
#include <Rendering/Renderable/RenderableGroup.h>
#include "winnt.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
    : m_screenWidth(350)
    , m_screenHeight(200)
    , m_shaderLibrary()
    , m_lastMousePos()
    , m_cameraPhi(0.f)
    , m_cameraTheta(0.f * XM_PI)
    , m_cameraOriginPos(XMVectorSet(0,0,-10, 1))
    , m_lookDir( XMVectorSet(0,0,1,0) )
{
    //m_deviceResources = std::make_unique<DX::DeviceResources>();
    //m_deviceResources->RegisterDeviceNotify(this);
    m_renderer = std::make_unique<RendererDX12>();
}

Game::~Game()
{
    m_renderer->Shutdown();
    m_renderer.reset();
    Shutdown();
    //if (m_deviceResources)
    //{
    //    m_deviceResources->WaitForGpu();
    //}
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    OnWindowSizeChanged(width, height);
    m_renderer->Init( window, width, height);

    Create_const_uav_srv_BufferDescriptorHeaps();
    LoadSceneData();

    BuildFrameResources();
    CreateConstantBufferViews();
    CreateComputableObjectsStructuredBufferViews();
    BuildRootSignature();
    BuildShaders(m_shaderLibrary);
    BuildPipelineStateObject();


    CreatePasses(m_shaderLibrary);

    m_renderer->FinaliseInit();

    //m_deviceResources->SetWindow(window, width, height);

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

void Game::Shutdown()
{

}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick(float totalTime, float deltaTime)
{
    m_totalTime = totalTime;
    Update(deltaTime);
    Render(deltaTime);
}

// Updates the world.
void Game::Update(float /*deltaTime*/)
{
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render(float /*deltaTime*/)
{
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    //auto r = m_deviceResources->GetOutputSize();
    //m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    m_screenWidth = width;
    m_screenHeight = height;

    //if (!m_deviceResources->WindowSizeChanged(width, height))
    //    return;

    //CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}


void Game::OnMouseDown(WPARAM /*btnState*/, int x, int y)
{
    m_lastMousePos.x = x;
    m_lastMousePos.y = y;
   
}

void Game::OnMouseUp(WPARAM /*btnState*/, int /*x*/, int /*y*/)
{
}

void Game::OnMouseMove(WPARAM mouseBtnState, int x, int y)
{
    if ((mouseBtnState & MK_LBUTTON) != 0)
    {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));

        // Update angles based on input to orbit camera around box.
        m_cameraTheta += dx;
        m_cameraPhi += dy;

        // Restrict the angle m_cameraPhi.
        m_cameraPhi = AstroTools::Maths::Clamp(m_cameraPhi, -AstroTools::Maths::Pi-0.1f, AstroTools::Maths::Pi - 0.1f);

        m_lastMousePos.x = x;
        m_lastMousePos.y = y;
    }
}

void Game::OnKeyboardKey(KeyboardKey key)
{
    XMVECTOR Up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR RightDir = XMVector3Cross(Up, m_lookDir);
    switch (key)
    {
    case KeyboardKey::Forward:
        m_cameraOriginPos = XMVectorAdd(m_cameraOriginPos, m_lookDir * 2);
        break;
    case KeyboardKey::Left:
        m_cameraOriginPos = XMVectorSubtract(m_cameraOriginPos, RightDir * 2);
        break;
    case KeyboardKey::Right:
        m_cameraOriginPos = XMVectorAdd(m_cameraOriginPos, RightDir * 2);
        break;
    case KeyboardKey::Back:
        m_cameraOriginPos = XMVectorSubtract(m_cameraOriginPos, m_lookDir * 2);
        break;
    case KeyboardKey::Down:
        m_cameraOriginPos = XMVectorSubtract(m_cameraOriginPos, Up * 2);
        break;
    case KeyboardKey::Up:
        m_cameraOriginPos = XMVectorAdd(m_cameraOriginPos, Up * 2);
        break;
    case KeyboardKey::Reset:
        m_cameraOriginPos = XMVectorSet(0, 0, 0, 0);
        m_cameraPhi = 0.f;
        m_cameraTheta = 0.f * XM_PI;
        m_lookDir = XMVectorSet(0, 0, 1, 0);
        break;
    default:
        DX::astro_assert(false, "Key not handled");
    }
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 2048;
    height = 1080;
}

#pragma endregion
