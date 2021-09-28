//
// Game.h
//

#pragma once
#include "Common.h"
#include "Rendering/IRenderer.h"

// A basic game implementation that creates a D3D12 device and
// provides a game loop.
class Game final 
{
public:

    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick(float deltaTime);

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

private:

    void Update(/*Timer*/);
    void Render(float deltaTime);

    std::unique_ptr<IRenderer> m_renderer;
};
