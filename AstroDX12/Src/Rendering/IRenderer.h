#pragma once
class IRenderable;

class IRenderer
{
public:
	virtual ~IRenderer() {};
	virtual void Init() = 0;
	virtual void Render() = 0;
	virtual void AddRenderable(IRenderable* renderable) = 0;
	virtual void Shutdown() = 0;
};

