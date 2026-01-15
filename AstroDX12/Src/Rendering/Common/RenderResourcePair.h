#pragma once

#include <memory>

template<class ResourceType>
class RenderResourcePair
{
private:
    bool m_isPing = true;
    std::shared_ptr<ResourceType> m_ping;
    std::shared_ptr<ResourceType> m_pong;

public:
    RenderResourcePair(std::shared_ptr<ResourceType> ping, std::shared_ptr<ResourceType> pong)
        : m_isPing(true)
        , m_ping(std::move(ping))
        , m_pong(std::move(pong))
    {
    }

    virtual ~RenderResourcePair()
    {
    }

    void Swap()
    {
        m_isPing = !m_isPing;
    }

    ResourceType* GetInput() const
    {
        return m_isPing ? m_ping.get() : m_pong.get();
    }

    ResourceType* GetOutput() const
    {
        return m_isPing ? m_pong.get() : m_ping.get();
    }
};