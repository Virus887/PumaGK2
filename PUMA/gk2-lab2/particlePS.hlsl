Texture2D cloudMap : register(t0);
Texture2D opacityMap : register(t1);
SamplerState colorSampler : register(s0);

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 tex1 : TEXCOORD0;
    float age : OPACITY;
};



float4 main(PSInput i) : SV_TARGET
{
    float TimeToLive = 2.0f;
    float opacity = TimeToLive - i.age;
    
	float4 color = cloudMap.Sample(colorSampler, i.tex1);
    color = float4(1.0, 0.8, 0.0, color.a);
	
    float alpha = color.a * opacity;

    if (i.age == 0)
        discard;
    
    return float4(color.xyz, alpha);
};