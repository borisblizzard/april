/// @version 5.1

#include "PixelShader_Textured.hlsli"

float4 main(PixelShaderInput input) : SV_Target
{
	float4 tex = cTexture.Sample(cSampler, input.tex);
	return float4(lerp(tex.rgb, input.color.rgb, input.lerpAlpha.a), tex.a * input.color.a);
}
