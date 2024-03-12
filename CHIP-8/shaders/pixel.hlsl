struct PixelShaderInput
{
    float2 uv : TEXCOORD0;
};
float4 SimplePixelShader( PixelShaderInput IN ) : SV_TARGET
{
    //return float4(IN.uv.xy,0,1);
    return float4(1,0,1,1);
}