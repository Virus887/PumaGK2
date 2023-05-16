cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
	matrix invViewMatrix;
};

cbuffer cbProj : register(b2) //Vertex Shader constant buffer slot 2
{
	matrix projMatrix;
};

struct VSInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL0;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 tex: TEXCOORD0;
};

PSInput main(VSInput i)
{
	PSInput o = (PSInput)0;
	o.pos = float4(i.pos, 1.0f);
	o.pos = mul(worldMatrix, o.pos);

	// TODO : 1.23 Calculate cube map texture coordinates
	// kierunek z czajnika (czyli tam gdzie kamera siedzi) do danej pozycji odibty za pomoca reflect
	float3 world_center = mul(invViewMatrix, float4(0, 0, 0, 1)).xyz;
	float3 world_norm = mul(worldMatrix, i.norm);
	world_norm = normalize(world_norm);
	float3 direction = i.pos - world_center;
	direction = normalize(direction);

	o.tex = reflect(direction, world_norm);

	o.pos = mul(viewMatrix, o.pos);
	o.pos = mul(projMatrix, o.pos);

	return o;
}