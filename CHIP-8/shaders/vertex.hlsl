struct VertexShaderOutput
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};
float4x4 CreateOrthographicMatrix(float left, float right, float bottom, float top, float nearClip, float farClip)
{
    float4x4 projectionMatrix;

    projectionMatrix[0][0] = 2.0f / (right - left);
    projectionMatrix[0][1] = 0.0f;
    projectionMatrix[0][2] = 0.0f;
    projectionMatrix[0][3] = 0.0f;

    projectionMatrix[1][0] = 0.0f;
    projectionMatrix[1][1] = 2.0f / (top - bottom);
    projectionMatrix[1][2] = 0.0f;
    projectionMatrix[1][3] = 0.0f;

    projectionMatrix[2][0] = 0.0f;
    projectionMatrix[2][1] = 0.0f;
    projectionMatrix[2][2] = 1.0f / (farClip - nearClip);
    projectionMatrix[2][3] = 0.0f;

    projectionMatrix[3][0] = (left + right) / (left - right);
    projectionMatrix[3][1] = (bottom + top) / (bottom - top);
    projectionMatrix[3][2] = nearClip / (nearClip - farClip);
    projectionMatrix[3][3] = 1.0f;

    return projectionMatrix;
}
VertexShaderOutput SimpleVertexShader( uint id: SV_VertexID)
{
    VertexShaderOutput OUT;
    
    
    float right = 516;
    float top= 439;
    // Define vertices for a fullscreen quad without using indices
    float2 vertices[6] = { 
        float2(-1.0, 1.0), float2(1.0, 1.0), float2(-1.0, -1.0),
        float2(1.0, 1.0), float2(1.0, -1.0), float2(-1.0, -1.0)
    };
    vertices[0].xy = float2(0,top);
    vertices[1].xy = float2(right,top);
    vertices[2].xy = float2(0,0);
    vertices[3].xy = float2(right,top);
    vertices[4].xy = float2(right,0);
    vertices[5].xy = float2(0,0);
    //glTexCoord2d(0.0, 0.0);		glVertex2d(0.0, 0.0);
	//glTexCoord2d(1.0, 0.0); 	glVertex2d(display_width, 0.0);
	//glTexCoord2d(1.0, 1.0); 	glVertex2d(display_width, display_height);
	//glTexCoord2d(0.0, 1.0); 	glVertex2d(0.0, display_height);
    float2 uv[6] = 
    {
    float2(0.0,1.0),float2(1.0,1.0),float2(0.0,0.0),
    float2(1.0,1.0),float2(1.0,0.0),float2(0.0,0.0)

    };


   float4x4 projectionMatrix = CreateOrthographicMatrix(0,right,0,top,0,100);

    // Set vertex position
    OUT.position = mul( float4(vertices[id], 0.0, 1.0),projectionMatrix);
    OUT.uv  = uv[id];
 
    return OUT;
}