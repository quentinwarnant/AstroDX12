#pragma once

#include <memory>
#include <Rendering/Common/RenderTarget.h>

class RenderTargetPair
{
private:
    bool m_isPing = true;
    std::shared_ptr<RenderTarget> m_ping;
    std::shared_ptr<RenderTarget> m_pong;

public:
    RenderTargetPair(std::shared_ptr<RenderTarget> ping, std::shared_ptr<RenderTarget> pong)
        : m_isPing(true)
        , m_ping(std::move(ping))
        , m_pong(std::move(pong))
    {
    }
    void Swap()
    {
        m_isPing = !m_isPing;
    }

    RenderTarget* GetInput() const
    {
        return m_isPing ? m_ping.get() : m_pong.get();
    }

    RenderTarget* GetOutput() const
    {
        return m_isPing ? m_pong.get() : m_ping.get();
    }
};