#include "objectHF.hlsli"
#include "skyHF.hlsli"

float4 main(float4 pos : SV_POSITION, float2 clipspace : TEXCOORD) : SV_TARGET
{
	float4 unprojected = mul(g_xCamera.InvVP, float4(clipspace, 0.0f, 1.0f));
	unprojected.xyz /= unprojected.w;

	const float3 V = normalize(unprojected.xyz - g_xCamera.CamPos);

	float4 color = float4(GetDynamicSkyColor(V), 1);

	return color;
}
