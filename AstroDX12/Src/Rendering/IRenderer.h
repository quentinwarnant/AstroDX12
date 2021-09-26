#pragma once

#include "../Common.h"

class IRenderable;

class IRenderer
{
public:
	virtual ~IRenderer() {};
	virtual void Init(HWND window, int width, int height) = 0;
	virtual void Render() = 0;
	virtual void AddRenderable(IRenderable* renderable) = 0;
	virtual void FlushRenderQueue() = 0;
	virtual void Shutdown() = 0;
};

