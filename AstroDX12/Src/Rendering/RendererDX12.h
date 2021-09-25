#pragma once
#include "IRenderer.h"

class IRenderable;

class RendererDX12 : public IRenderer
{
    // IRenderer - BEGIN
    virtual void Init() override;
    virtual void Render() override;
    virtual void AddRenderable(IRenderable* renderable) override;
    virtual void ShutDown() override;
    // IRenderer - END
};

