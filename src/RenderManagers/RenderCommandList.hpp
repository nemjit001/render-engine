#pragma once
#ifndef RENDER_COMMAND_LIST_HPP
#define RENDER_COMMAND_LIST_HPP

#include <memory>
#include <vector>
#include "RenderManager.hpp"

/// @brief The RenderCommandExecutor interface implements command execution for an underlying graphics backend.
class IRenderCommandExecutor
{
public:
    virtual ~IRenderCommandExecutor() = default;

    virtual void CopyBufferToBuffer(
        GPUBufferHandle src, GPUBufferHandle dst,
        size_t srcOffset, size_t dstOffset, size_t size
    ) = 0;

    virtual void CopyBufferToTexture(
        GPUBufferHandle src, GPUTextureHandle dst,
        size_t bufferOffset, uint32_t rowPitch, uint32_t rowCount,
        TextureSubresource subresource, Offset3D textureOffset, Extent3D textureExtent
    ) = 0;
};

/// @brief The RenderCommand interface implements an executable render command.
class IRenderCommand
{
public:
    virtual ~IRenderCommand() = default;

    /// @brief Execute the render command using a render command executor.
    /// @param executor Executor to use for command execution.
    virtual void Execute(IRenderCommandExecutor* executor) const = 0;
};

/// @brief The internal RenderCommandList implementation.
class RenderCommandList : public IRenderCommandList
{
public:
    void CopyBufferToBuffer(
        GPUBufferHandle src, GPUBufferHandle dst,
        size_t srcOffset, size_t dstOffset, size_t size
    ) override;

    void CopyBufferToTexture(
        GPUBufferHandle src, GPUTextureHandle dst,
        size_t bufferOffset, uint32_t rowPitch, uint32_t rowCount,
        TextureSubresource subresource, Offset3D textureOffset, Extent3D textureExtent
    ) override;

    void Dispatch(IRenderCommandExecutor* executor) const override;

private:
    std::vector<std::unique_ptr<IRenderCommand>> _renderCommands;
};

#endif //RENDER_COMMAND_LIST_HPP
