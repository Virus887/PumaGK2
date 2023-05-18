cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
    matrix viewMatrix;
};

struct VSInput
{
    float3 prevPos : POSITION0;
    float3 pos : POSITION1;
    float age : TEXCOORD0;
    float size : TEXCOORD1;
};

struct GSInput
{
    float4 prevPos : POSITION0;
    float4 pos : POSITION1;
    float age : TEXCOORD0;
    float size : TEXCOORD1;
};

GSInput main(VSInput i)
{
    GSInput o = (GSInput) 0;
    o.prevPos = float4(i.prevPos, 1.0f);
    o.prevPos = mul(viewMatrix, o.prevPos);
    o.pos = float4(i.pos, 1.0f);
    o.pos = mul(viewMatrix, o.pos);
    o.age = i.age;
    o.size = i.size;
    return o;
}