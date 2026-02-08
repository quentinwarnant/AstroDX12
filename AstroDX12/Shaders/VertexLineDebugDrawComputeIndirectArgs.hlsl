cbuffer BindlessRenderResources : register(b0)
{
    int counterBufferIdx;
    int drawArgBufferIdx;
    int lineVertexBufferIdx;
	int globalCBVIdx;
};

struct D3D12_DRAW_ARGUMENTS
{
    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

struct IndirectArgs
{
    int lineVertexBufferIdx;
	int globalCBVIdx;
    
    D3D12_DRAW_ARGUMENTS drawArgs;
};

[numthreads(1, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    RWByteAddressBuffer CounterBuffer = ResourceDescriptorHeap[counterBufferIdx];
    RWStructuredBuffer<IndirectArgs> ArgumentBuffer = ResourceDescriptorHeap[drawArgBufferIdx];

    uint lineCount = CounterBuffer.Load(0);
    
    IndirectArgs args;
    args.lineVertexBufferIdx = lineVertexBufferIdx;
    args.globalCBVIdx = globalCBVIdx;
    
    args.drawArgs.VertexCountPerInstance = lineCount * 2; // VertexCount
    args.drawArgs.InstanceCount = 1;                      // InstanceCount
    args.drawArgs.StartVertexLocation = 0;                // StartVertexLocation
    args.drawArgs.StartInstanceLocation = 0;              // StartInstanceLocation
    ArgumentBuffer[0] = args;
    
    // Reset lineCountBuffer for next frame
    CounterBuffer.Store(0, 0);

}