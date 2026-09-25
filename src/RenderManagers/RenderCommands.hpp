#pragma once
#ifndef RENDER_COMMANDS_HPP
#define RENDER_COMMANDS_HPP

#include "RenderCommandList.hpp"

/// @brief Command for copying a buffer to a buffer.
class CopyBufferToBufferCommand : public IRenderCommand
{
public:
    constexpr CopyBufferToBufferCommand(
        GPUBufferHandle src, GPUBufferHandle dst,
        size_t srcOffset, size_t dstOffset, size_t size
    )
        : _srcBuffer(src), _dstBuffer(dst),
            _srcOffset(srcOffset), _dstOffset(dstOffset), _size(size) {}

    void Execute(IRenderCommandExecutor* executor) const override;

private:
    GPUBufferHandle _srcBuffer = nullptr;
    GPUBufferHandle _dstBuffer = nullptr;
    size_t _srcOffset = 0;
    size_t _dstOffset = 0;
    size_t _size = 0;
};

/// @brief Command for copying a buffer to a texture subresource.
class CopyBufferToTextureCommand : public IRenderCommand
{
public:
    constexpr CopyBufferToTextureCommand(
        GPUBufferHandle src, GPUTextureHandle dst,
        size_t bufferOffset, uint32_t rowPitch, uint32_t rowCount,
        TextureSubresource subresource, Offset3D textureOffset, Extent3D textureExtent
    )
        : _srcBuffer(src), _dstTexture(dst), _bufferOffset(bufferOffset), _rowPitch(rowPitch), _rowCount(rowCount),
            _subresource(subresource), _textureOffset(textureOffset), _textureExtent(textureExtent) {}

    void Execute(IRenderCommandExecutor* executor) const override;

private:
    GPUBufferHandle _srcBuffer = nullptr;
    GPUTextureHandle _dstTexture = nullptr;
    size_t _bufferOffset = 0;
    uint32_t _rowPitch = 0;
    uint32_t _rowCount = 0;
    TextureSubresource _subresource = {};
    Offset3D _textureOffset = {};
    Extent3D _textureExtent = {};
};

#endif //RENDER_COMMANDS_HPP
