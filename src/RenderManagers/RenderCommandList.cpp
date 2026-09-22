#include "RenderCommandList.hpp"

void RenderCommandList::CopyBufferToBuffer(GPUBufferHandle src, GPUBufferHandle dst, size_t srcOffset, size_t dstOffset, size_t size)
{
    //
}
    
void RenderCommandList::CopyBufferToTexture(
    GPUBufferHandle src, GPUTextureHandle dst,
    size_t bufferOffset, uint32_t rowPitch, uint32_t rowCount,
    TextureSubresource subresource, Offset3D textureOffset, Extent3D textureExtent
)
{
    //
}

void RenderCommandList::Execute(IRenderCommandExecutor* executor)
{
    //
}
