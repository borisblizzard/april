/// @version 4.0

cbuffer constantBuffer : register(b0)
{
	min16float4x4 cMatrix;
	min16float4 cSystemColor;
	min16float4 cLerpAlpha;
};

