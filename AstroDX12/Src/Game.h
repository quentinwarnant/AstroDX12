//
// Game.h
//

#pragma once
#include <Common.h>
#include <Input/KeyboardInput.h>
#include <Rendering/IRenderer.h>
#include <Rendering/Renderable/IRenderable.h>
#include <Rendering/Common/ShaderLibrary.h>

class IRenderable;

// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game 
{
public:
	Game() noexcept(false);
    virtual ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and resource management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick(float totalTime, float deltaTime);

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);
    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM mouseBtnState, int x, int y);
    void OnKeyboardKey(KeyboardKey key);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;
    inline float GetAspectRatio() const { return (float)m_screenWidth / (float)m_screenHeight;  }
    inline float GetScreenWidth() const { return (float)m_screenWidth; }
    inline float GetScreenHeight() const { return (float)m_screenHeight; }
    inline float GetTotalTime() const { return m_totalTime; }

    // Scene renderable objects building
    virtual void BuildFrameResources() = 0;
    virtual void Create_const_uav_srv_BufferDescriptorHeaps() = 0;
    virtual void CreateConstantBufferViews() = 0;
    virtual void CreateComputableObjectsStructuredBufferViews() = 0;
    virtual void BuildRootSignature() = 0;
    virtual void BuildShaders(AstroTools::Rendering::ShaderLibrary& shaderLibrary) = 0;
    virtual void BuildPipelineStateObject() = 0;

protected:
    // Triggered after Initialise, meant for initial state configuration 
    virtual void LoadSceneData() = 0;
    virtual void CreatePasses(AstroTools::Rendering::ShaderLibrary& shaderLibrary) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(float deltaTime) = 0;
    virtual void Shutdown() = 0;
   
    std::unique_ptr<IRenderer> m_renderer;

private:
    int m_screenWidth;
    int m_screenHeight;
    float m_totalTime;

protected:
    POINT m_lastMousePos;
    float m_cameraTheta;
    float m_cameraPhi;
    XMVECTOR m_cameraOriginPos;
    XMVECTOR m_lookDir;
private:
    AstroTools::Rendering::ShaderLibrary m_shaderLibrary;
};
