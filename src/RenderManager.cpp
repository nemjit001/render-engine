#include "RenderManager.hpp"

RenderManager& RenderManager::Get()
{
    static RenderManager instance{};
    return instance;
}

bool RenderManager::Init()
{
    return false;
}

void RenderManager::Shutdown()
{
    //
}

void RenderManager::ProcessEvent(SDL_Event const& event)
{
    //
}

void RenderManager::Frame()
{
    //
}
