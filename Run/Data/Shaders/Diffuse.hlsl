//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 modelPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 modelTangent : TANGENT;
	float3 modelBitangent : BITANGENT;
	float3 modelNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 clipPosition : SV_Position;
	float4 worldPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 worldTangent : TANGENT;
	float4 worldBitangent : BITANGENT;
	float4 worldNormal : NORMAL;
};

// -----------------------------------------------------------------------------------------------
struct PointLight
{
	float4 Position;
	float4 Color;
};

#define MAX_POINT_LIGHTS 64
// -----------------------------------------------------------------------------------------------
struct SpotLight
{
	float4 Position;
	float  InnerRadius;
    float  OuterRadius;
    float  InnerPenumbra;
    float  OuterPenumbra;
    float  InnerPenumbraDotThreshold;
    float  OuterPenumbraDotThreshold;
    float4 Color;
};
#define MAX_SPOT_LIGHTS 8
//------------------------------------------------------------------------------------------------
cbuffer PerFrameConstants : register(b1)
{
	float		c_time;
	int			c_debugInt;
	float		c_debugFloat;
	int			EMPTY_PADDING;
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 WorldToCameraTransform;	// View transform
	float4x4 CameraToRenderTransform;	// Non-standard transform from game to DirectX conventions
	float4x4 RenderToClipTransform;		// Projection transform
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelToWorldTransform;		// Model transform
	float4 ModelColor;
};
//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b4)
{
	float3 SunDirection;
	float SunIntensity;
	float AmbientIntensity;
	float3  padders;

	int NumPointLights;
	float3 pointPadders;
	PointLight PointLights[MAX_POINT_LIGHTS];

	int NumSpotLights;
	float3 spotPadders;
	SpotLight  SpotLights[MAX_SPOT_LIGHTS];
};
//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);

//------------------------------------------------------------------------------------------------
SamplerState samplerState : register(s0);

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 modelPosition = float4(input.modelPosition, 1);
	float4 worldPosition = mul(ModelToWorldTransform, modelPosition);
	float4 cameraPosition = mul(WorldToCameraTransform, worldPosition);
	float4 renderPosition = mul(CameraToRenderTransform, cameraPosition);
	float4 clipPosition = mul(RenderToClipTransform, renderPosition);

	float4 worldTangent = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));
	float4 worldBitangent = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));
	float4 worldNormal = mul(ModelToWorldTransform, float4(input.modelNormal, 0.0f));

	v2p_t v2p;
	v2p.clipPosition = clipPosition;
	v2p.worldPosition = worldPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.worldTangent = worldTangent;
	v2p.worldBitangent = worldBitangent;
	v2p.worldNormal = worldNormal;
	return v2p;
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 textureColor = diffuseTexture.Sample(samplerState, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;

	float4 ambient = AmbientIntensity * float4(1.0f, 1.0f, 1.0f, 1.0f);
	float4 directional = SunIntensity * saturate(dot(normalize(input.worldNormal.xyz), -SunDirection)) * float4(1.0f, 1.0f, 1.0f, 1.0f);
	float4 lightColor = ambient + directional;
	
	//-----------------------------------------POINT LIGHTS---------------------------------------------------//
	for (int lightIndex = 0; lightIndex < NumPointLights; ++lightIndex)
	{
		float4 pixelToLight = PointLights[lightIndex].Position - input.worldPosition;
		float distance = length(pixelToLight);
		pixelToLight = normalize(pixelToLight);
		float linearfalloff = 0.09f;
		float quadratic = 0.032f;
		float attenuation = 1.0f / (1.0f + linearfalloff * distance + quadratic * distance * distance);
		lightColor += PointLights[lightIndex].Color * attenuation * saturate(dot(normalize(input.worldNormal), pixelToLight));
	}
	//-------------------------------------------------------------------------------------------------------//

	float4 color = lightColor * textureColor * vertexColor * modelColor;
	clip(color.a - 0.01f);
	return color;
}
