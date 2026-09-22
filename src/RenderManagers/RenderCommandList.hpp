#pragma once
#ifndef RENDER_COMMAND_LIST_HPP
#define RENDER_COMMAND_LIST_HPP

#include "RenderManager.hpp"

/// @brief The RenderCommandExecutor interface implements actual command forwarding to an underlying graphics backend.
class IRenderCommandExecutor
{
public:
    virtual ~IRenderCommandExecutor() = default;
};

/// @brief The internal RenderCommandList implementation.
class RenderCommandList : public IRenderCommandList
{
public:
    void CopyBufferToBuffer(GPUBufferHandle src, GPUBufferHandle dst, size_t srcOffset, size_t dstOffset, size_t size) override;

    void CopyBufferToTexture(
        GPUBufferHandle src, GPUTextureHandle dst,
        size_t bufferOffset, uint32_t rowPitch, uint32_t rowCount,
        TextureSubresource subresource, Offset3D textureOffset, Extent3D textureExtent
    ) override;

    void Execute(IRenderCommandExecutor* executor) override;
};

#endif //RENDER_COMMAND_LIST_HPP
