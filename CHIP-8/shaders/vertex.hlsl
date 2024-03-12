struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VertexShaderOutput SimpleVertexShader( uint id: SV_VertexID)
{
    VertexShaderOutput OUT;
    
    
    // Define vertices for a fullscreen quad without using indices
    float2 vertices[6] = { 
        float2(-1.0, 1.0), float2(1.0, 1.0), float2(-1.0, -1.0),
        float2(1.0, 1.0), float2(1.0, -1.0), float2(-1.0, -1.0)
    };

    // Set vertex position
    OUT.position = float4(vertices[id ], 0.0, 1.0);
    OUT.uv  = float2(1,1);
 
    return OUT;
}