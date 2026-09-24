#include "RenderCommandList.hpp"

#include "RenderManagers/RenderCommands.hpp"

void RenderCommandList::CopyBufferToBuffer(GPUBufferHandle src, GPUBufferHandle dst, size_t srcOffset, size_t dstOffset, size_t size)
{
    _renderCommands.push_back(std::make_unique<CopyBufferToBufferCommand>(src, dst, srcOffset, dstOffset, size));
}
    
void RenderCommandList::CopyBufferToTexture(
    GPUBufferHandle src, GPUTextureHandle dst,
    size_t bufferOffset, uint32_t rowPitch, uint32_t rowCount,
    TextureSubresource subresource, Offset3D textureOffset, Extent3D textureExtent
)
{
    _renderCommands.push_back(std::make_unique<CopyBufferToTextureCommand>(src, dst, bufferOffset, rowPitch, rowCount, subresource, textureOffset, textureExtent));
}

void RenderCommandList::Dispatch(IRenderCommandExecutor* executor) const
{
    for (auto const& command : _renderCommands) {
        command->Execute(executor);
    }
}
