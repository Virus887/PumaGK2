cbuffer cbProj : register(b0) //Geometry Shader constant buffer slot 0
{
    matrix projMatrix;
};

struct GSInput
{
    float4 prevPos : POSITION0;
    float4 pos : POSITION1;
    float age : TEXCOORD0;
    float size : TEXCOORD1;
};
struct PSInput
{
    float4 pos : SV_POSITION;
    float2 tex1 : TEXCOORD0;
    float age : OPACITY;
};

[maxvertexcount(4)]
void main(point GSInput inArray[1], inout TriangleStream<PSInput> ostream)
{
    GSInput i = inArray[0];
    
    float dy = 0.015f;

    PSInput o = (PSInput) 0;
	

	[branch]
    if (i.pos.y < i.prevPos.y)
    {
        dy = -dy;
    }

    o.pos = i.pos;
    o.pos = mul(projMatrix, o.pos);
    o.tex1 = float2(0.0f, 1.0f);
    o.age = i.age;
    ostream.Append(o);

    o.pos = i.pos + float4(dy, 0, 0.0f, 0.0f);
    o.pos = mul(projMatrix, o.pos);
    o.tex1 = float2(1.0f, 1.0f);
    o.age = i.age;
    ostream.Append(o);

    o.pos = i.prevPos;
    o.pos = mul(projMatrix, o.pos);
    o.tex1 = float2(0.0f, 0.0f);
    o.age = i.age;
    ostream.Append(o);

    o.pos = i.prevPos + float4(dy, 0, 0.0f, 0.0f);
    o.pos = mul(projMatrix, o.pos);
    o.tex1 = float2(1.0f, 0.0f);
    o.age = i.age;
    ostream.Append(o);
	
    ostream.RestartStrip();
}