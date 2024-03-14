Texture2D textur;
SamplerState sample;

struct PixelShaderInput
{
    float2 uv : TEXCOORD0;
};
float4 SimplePixelShader( PixelShaderInput IN ) : SV_TARGET
{
    float2 uv = float2(IN.uv.x,1-IN.uv.y);
    float4 tex = textur.Sample(sample,uv);
    return tex;
    return float4(IN.uv.xy,0,1);
    return float4(1,0,1,1);
}