#pragma once
#ifndef RENDER_MANAGER_HPP
#define RENDER_MANAGER_HPP

#include <cstdint>
#include <SDL3/SDL.h>

#ifndef NDEBUG
static constexpr bool RENDERER_ENABLE_DEBUG = true;
#else
static constexpr bool RENDERER_ENABLE_DEBUG = false;
#endif //NDEBUG

class GPUBuffer;
class GPUTexture;

typedef GPUBuffer* GPUBufferHandle;
typedef GPUTexture* GPUTextureHandle;

/// @brief Enumeration of GPU heap types.
enum GPUHeapType : uint32_t
{
    GPUHeapType_Default     = 0,
    GPUHeapType_Upload      = 1,
    GPUHeapType_Readback    = 2,
};

/// @brief Enumeration of buffer usage flag bits.
enum BufferUsageFlagBits : uint32_t
{
    BufferUsage_TransferSrc     = 0x00000001,
    BufferUsage_TransferDst     = 0x00000002,
    BufferUsage_UniformBuffer   = 0x00000004,
    BufferUsage_StorageBuffer   = 0x00000008,
    BufferUsage_IndexBuffer     = 0x00000010,
    BufferUsage_VertexBuffer    = 0x00000020,
    BufferUsage_IndirectBuffer  = 0x00000040,
};
typedef uint32_t BufferUsageFlags;

/// @brief Enumeration of texture usage flag bits.
enum TextureUsageFlagBits : uint32_t
{
    TextureUsage_TransferSrc            = 0x00000001,
    TextureUsage_TransferDst            = 0x00000002,
    TextureUsage_SampledImage           = 0x00000004,
    TextureUsage_StorageImage           = 0x00000008,
    TextureUsage_RenderAttachment       = 0x00000010,
    TextureUsage_DepthStencilAttachment = 0x00000020,
};
typedef uint32_t TextureUsageFlags;

/// @brief Initialization info for the render manager.
struct RenderManagerInitInfo
{
    char const* windowTitle = "App";    //< Default window title.
    uint32_t windowWidth    = 1280u;    //< Initial window width.
    uint32_t windowHeight   = 720u;     //< Initial window height.
    uint32_t framesInFlight = 2u;       //< Number of frames that may be recorded simultaneously, lower values means lower frame latency, values in the range [1, 3] are recommended.
};

/// @brief GPU buffer description.
struct GPUBufferDesc
{
    GPUHeapType heapType    = GPUHeapType_Default;
    uint32_t size           = 0u;
    BufferUsageFlags usage  = 0u;
};

/// @brief GPU texture description.
struct GPUTextureDesc
{
    GPUHeapType heapType    = GPUHeapType_Default;
    uint32_t width          = 0u;
    uint32_t height         = 0u;
    uint32_t depthOrLayers  = 0u;
    uint32_t mipLevels      = 0u;
    uint32_t sampleCount    = 1u;
    TextureUsageFlags usage = 0u;
};

/// @brief The RenderManager interface for managing render resources and frame submission can be implemented to support different render backends.
class IRenderManager
{
public:
    IRenderManager() = default;
    virtual ~IRenderManager() = default;

    IRenderManager(IRenderManager const&) = delete;
    IRenderManager& operator=(IRenderManager const&) = delete;

    /// @brief Initialize the render manager.
    /// @param initInfo Initialization info.
    /// @return A boolean indicating success.
    [[nodiscard]]
    virtual bool Init(RenderManagerInitInfo const& initInfo) = 0;

    /// @brief Shut down the render manager.
    virtual void Shutdown() = 0;

    /// @brief Process a platform event.
    /// @param event Event to process.
    virtual void ProcessEvent(SDL_Event const& event) = 0;

    [[nodiscard]]
    virtual GPUBufferHandle CreateGPUBuffer(GPUBufferDesc const& bufferDesc) = 0;

    [[nodiscard]]
    virtual GPUTextureHandle CreateGPUTexture(GPUTextureDesc const& textureDesc) = 0;

    virtual void DestroyGPUBuffer(GPUBufferHandle buffer) = 0;

    virtual void DestroyGPUTexture(GPUTextureHandle texture) = 0;

    /// @brief Start a new frame.
    /// @return A boolean indicating successful frame start.
    [[nodiscard]]
    virtual bool NewFrame() = 0;

    /// @brief End the current frame.
    virtual void EndFrame() = 0;

    /// @brief Execute the frame commands for the current frame.
    virtual void ExecuteFrame() const = 0;

    /// @brief Wait for the graphics device to be idle.
    virtual void WaitIdle() const = 0;

    /// @brief Get the current frame index.
    /// @return The current frame index.
    [[nodiscard]]
    virtual uint64_t GetCurrentFrameIndex() const = 0;
    
    /// @brief Get the current frame in flight index in the range [0, frames in flight].
    /// @return The frame in flight index.
    [[nodiscard]]
    virtual uint64_t GetCurrentFrameInFlightIndex() const = 0;
};

/// @brief Check if a bit is set in a bitflag type.
/// @tparam FlagType Flags type.
/// @tparam BitFlagType Bit flag type.
/// @param flags Set flags.
/// @param bit Bit to test.
/// @return A boolean indicating if a bit is set in the bitflags.
template<typename FlagType, typename BitFlagType>
static constexpr bool IsBitFlagSet(FlagType flags, BitFlagType bit)
{
    return flags & bit;
}

#endif //RENDER_MANAGER_HPP
