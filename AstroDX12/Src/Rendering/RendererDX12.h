#pragma once
#include "IRenderer.h"
#include "../Common.h"

using namespace Microsoft::WRL;

class IRenderable;

class RendererDX12 : public IRenderer
{
public:
    virtual ~RendererDX12();

    // IRenderer - BEGIN
    virtual void Init() override;
    virtual void Render() override;
    virtual void AddRenderable(IRenderable* renderable) override;
    virtual void Shutdown() override;
    // IRenderer - END

private: 
    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12Device> m_device;

};

