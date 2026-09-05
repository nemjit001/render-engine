#pragma once
#ifndef RENDER_TYPES_HPP
#define RENDER_TYPES_HPP

#include <cstdint>

enum class RenderBackend : uint32_t
{
    Vulkan = 0,
};

/// @brief GPU buffer base class.
class GPUBuffer
{
public:
    GPUBuffer() = default;
    virtual ~GPUBuffer() = default;

    /// @brief Get the render backend for this buffer.
    /// @return
    [[nodiscard]]
    virtual RenderBackend GetRenderBackend() const = 0;
};

/// @brief GPU texture base class.
class GPUTexture
{
public:
    GPUTexture() = default;
    virtual ~GPUTexture() = default;

    /// @brief Get the render backend for this buffer.
    /// @return
    [[nodiscard]]
    virtual RenderBackend GetRenderBackend() const = 0;
};

#endif //RENDER_TYPES_HPP
