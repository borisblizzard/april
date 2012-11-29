struct PixelShaderInput
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float4 colorModeData : COLOR1; // r is color-mode, g is color-mode-alpha (used in LERP), b and a are not used
};

float4 main(PixelShaderInput input) : SV_Target
{
	float4 result = {1.0f, 1.0f, 1.0f, 1.0f};
	if (input.colorModeData.r > 1.999 && input.colorModeData.r < 2.001) // LERP
	{
		result = float4(lerp(result.rgb, input.color.rgb, input.color.a), result.a * input.colorModeData.g);
	}
	else if (input.colorModeData.r > 2.999 && input.colorModeData.r < 3.001) // ALPHA_MAP
	{
		result = float4(input.color.rgb, input.color.a * result.r);
	}
	else // MULTIPLY
	{
		result *= input.color;
	}
	return result;
}
