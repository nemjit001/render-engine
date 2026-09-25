#include "RenderCommands.hpp"

void CopyBufferToBufferCommand::Execute(IRenderCommandExecutor* executor) const
{
    executor->CopyBufferToBuffer(_srcBuffer, _dstBuffer, _srcOffset, _dstOffset, _size);
}

void CopyBufferToTextureCommand::Execute(IRenderCommandExecutor* executor) const
{
    executor->CopyBufferToTexture(
        _srcBuffer, _dstTexture,
        _bufferOffset, _rowPitch, _rowCount,
        _subresource, _textureOffset, _textureExtent
    );
}
