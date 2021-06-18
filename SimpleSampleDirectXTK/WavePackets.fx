//***************************************************************************************
// 
// Defines Effect11 Rendering PipeLine Settings
// 
//***************************************************************************************

#include "GlobalDefs.h"

#define PI 3.14159


// ================================================================================
//
// Global variables
//
// ================================================================================

float	  g_BoxPos;
float4x4 g_mWorld;                  // World matrix for object
float4x4 g_mWorldViewProjection;    // View * Projection matrix
float	g_diffX;					// x size of grid texture
float	g_diffY;					// y size of grid texture

Texture2D g_waterTerrainTex;	// water depth and terrain height texture
Texture2D g_waterPosTex;		// position of water samples
Texture2D g_waterHeightTex;		// displacement of water samples

TextureCube g_TexCube;		//skybox

// ================================================================================
//
// Vertex and Pixel shader structures
//
// ================================================================================

struct VertexPos
{
	float3 PosL : POSITION;
};

struct VertexPosHL
{
	float4 PosH : SV_POSITION;
	float3 PosL : POSITION;
};

struct VS_CUBE_INPUT
{
	float3 PosW	:	POSITION;
	float4 Col	:	COLOR;
};

struct VS_CUBE_OUT
{
	float4 PosH :	SV_POSITION;
	float4 Col	:	COLOR;
};

struct VS_INPUT
{
    float2 PosTex		: POSITION;
};

struct VS_QUAD_INPUT
{
    float2 PosTex		: POSITION;
    float2 Tex		: TEXTURE0;
};

struct VS_PACKET_INPUT
{
	float4 PosV	: POSITION;  // (x,y) = world position of this vertex, z,w = direction of traveling
	float4 Att	: TEXTURE0;  // x = amplitude, w = time this ripple was initialized
	float4 Att2	: TEXTURE1;
};

struct VertexPosNormalTex
{
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex : TEXCOORD;
};

struct VertexPosHWNormalTex
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION; 
	float3 NormalW : NORMAL; 
	float2 Tex : TEXCOORD;
};

//--------------------------------------

struct PS_INPUT
{
	float4 PosH : SV_POSITION;
    float2 Tex		 : TEXTURE0;
};

struct PS_POS_INPUT
{
	float4 PosH : SV_POSITION;
	float2 Tex		 : TEXTURE0;
	float3 PosW		 : TEXTURE1; // world space position
};

struct PS_PACKET_INPUT
{
	float4 PosH	 : SV_POSITION;
	float4 PosW			 : TEXTURE0;
	float4 Att			 : TEXTURE1; 
	float4 Att2			 : TEXTURE2; 
};

struct PS_OUTPUT
{
	float4 Col	: SV_Target0;
};


// ================================================================================
//
// Samplers
//
// ================================================================================

SamplerState PointSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

// ================================================================================
//
// Vertex shader
//
// ================================================================================

VertexPosHL SkyVS(VertexPos vIn)
{
	VertexPosHL vOut;
	// 设置z = w使得z/w = 1(天空盒保持在远平面)
	float4 posH = mul(float4(vIn.PosL, 1.0f), g_mWorldViewProjection);
	vOut.PosH = posH.xyww;
	vOut.PosL = vIn.PosL;
	return vOut;
}

VS_CUBE_OUT CubeVS(VS_CUBE_INPUT In)
{
	VS_CUBE_OUT Out;
	Out.PosH = mul(float4(In.PosW.x,In.PosW.y+ g_BoxPos,In.PosW.z, 1.0f), g_mWorldViewProjection);
	Out.Col = In.Col;                  
	return Out;
}

VertexPosHWNormalTex VS_3D(VertexPosNormalTex vIn)
{
	VertexPosHWNormalTex vOut;
	float4 posW = mul(float4(vIn.PosL, 1.0f), g_mWorld);
	float3 normalW = mul(vIn.NormalL, (float3x3) g_mWorld);

	vOut.PosH = mul(float4(vIn.PosL, 1.0f), g_mWorldViewProjection);
	vOut.PosW = posW.xyz;
	vOut.NormalW = normalW;
	vOut.Tex = vIn.Tex;
	return vOut;
}

// this rasterizes position of the wave mesh on screel
VS_PACKET_INPUT PacketVS(VS_PACKET_INPUT In)
{
    return In;
}


PS_POS_INPUT DisplayWaveMeshVS(VS_INPUT In )
{
	PS_POS_INPUT Out;

	Out.PosW = 0.5*SCENE_EXTENT*float3(In.PosTex.x, 0, In.PosTex.y);
	Out.PosH = mul( float4(Out.PosW,1.0), g_mWorldViewProjection);
	Out.Tex = In.PosTex;

	return Out;
}


// takes a simple 2D vertex on the ground plane, offsets it along y by the land or water offset and projects it on screen
PS_POS_INPUT DisplayMicroMeshVS(VS_INPUT In)
{
	PS_POS_INPUT Out;
	Out.Tex = In.PosTex;
	float4 pos = g_waterPosTex.SampleLevel(PointSampler, In.PosTex.xy, 0.0);
	if (pos.w <= 0.0)  // if no position data has been rendered to this texel, there is no water surface here
	{
		Out.PosW = float3(0, 0, 0);
		Out.PosH = float4(0, 0, 0, -1.0);
	}
	else
	{
		Out.PosW = pos.xyz + g_waterHeightTex.SampleLevel(LinearSampler, In.PosTex.xy, 0).xyz;
		Out.PosH = mul(float4(Out.PosW.xyz, 1.0), g_mWorldViewProjection);
	}
	
	return Out;
}



// takes a simple 2D vertex on the ground plane, offsets it along y by the land or water offset and projects it on screen
PS_POS_INPUT DisplayTerrainVS(VS_INPUT In)
{
	PS_POS_INPUT Out;
	Out.Tex = 0.5 * (In.PosTex.xy + float2(1.0, 1.0));
	float Height = g_waterTerrainTex.SampleLevel(LinearSampler, Out.Tex, 0).y;
	Out.PosW = SCENE_EXTENT*0.5*float3(In.PosTex.x,Height-0.35, In.PosTex.y);
	Out.PosH = mul(float4(Out.PosW,1.0), g_mWorldViewProjection);
	return Out;
}



// this rasterizes a quad for full screen AA
PS_INPUT RenderQuadVS(VS_QUAD_INPUT In)
{
	PS_INPUT Out;
	Out.Tex = In.Tex;
	Out.PosH = float4(In.PosTex.xy, 0.5, 1.0);
	return Out;
}



// ================================================================================
//
// Geometry shader
//
// ================================================================================

[maxvertexcount(4)]
void PacketGS( point VS_PACKET_INPUT input[1], inout TriangleStream<PS_PACKET_INPUT> tStream )
{

	PS_PACKET_INPUT p0;
	p0.Att = input[0].Att.xyzw; 
	p0.Att2 = input[0].Att2.xyzw;
	float3 cPos = float3(input[0].PosV.x, 0, input[0].PosV.y );	// "dangling patch" center
	float3 depth = float3(input[0].PosV.z, 0, input[0].PosV.w );// vector in traveling direction (orthogonal to crest direction)
	float3 width =  float3(depth.z, 0, -depth.x);				// vector along wave crest (orthogonal to travel direction)
	float dThickness = input[0].Att.w;							// envelope size of packet (=depthspread)
	float wThickness = input[0].Att.w;							// rectangular constant sidewidth patches (but individual thickness = envelope size)
	float3 p1 = cPos + 0.0*depth		- wThickness*width;		// neighboring packet patches overlap by 50%
	float3 p2 = cPos - dThickness*depth - wThickness*width;
	float3 p3 = cPos + 0.0*depth		+ wThickness*width;
	float3 p4 = cPos - dThickness*depth + wThickness*width;
	p0.PosW = float4(-1,1, -input[0].Att.w, 0);
	p0.PosH = mul(float4(p1,1.0), g_mWorldViewProjection);
	tStream.Append( p0 );
	p0.PosW = float4(-1,-1, -input[0].Att.w, -input[0].Att.w);
	p0.PosH = mul(float4(p2,1.0), g_mWorldViewProjection);
	tStream.Append( p0 );
	p0.PosW = float4(1,1, input[0].Att.w, 0);
	p0.PosH = mul(float4(p3,1.0), g_mWorldViewProjection);
	tStream.Append( p0 );
	p0.PosW = float4(1,-1, input[0].Att.w, -input[0].Att.w);
	p0.PosH = mul(float4(p4,1.0), g_mWorldViewProjection);
	tStream.Append( p0 );
	tStream.RestartStrip(); 
}


[maxvertexcount(3)]
void DisplayMicroMeshGS( triangle PS_POS_INPUT input[3], inout TriangleStream<PS_POS_INPUT> tStream )
{
	if ((input[0].PosH.w<0.01) || (input[1].PosH.w<0.01) || (input[2].PosH.w<0.01))
		return;
	tStream.Append(input[0]);
	tStream.Append(input[1]);
	tStream.Append(input[2]);
}


// ================================================================================
//
// Pixel Shaders
//
// ================================================================================

float4 SkyPS(VertexPosHL pIn) : SV_Target
{
	return g_TexCube.Sample(LinearSampler, pIn.PosL);
}

float4 CubePS(VS_CUBE_OUT In) :SV_Target
{
	return In.Col;
}

float4 PS_3D(VertexPosHWNormalTex pIn) : SV_Target
{
	return float4(1.0,1.0,1.0,1.0);
}

// rasterize wave packet quad
// wave packet data:
// position vector: x,y = [-1..1], position in envelope
// attribute vector: x=amplitude, y=wavelength, z=time phase, w=envelope size
// attribute2 vector: (x,y)=position of bending point, z=central distance to ref point, 0
PS_OUTPUT PacketDisplacementPS(PS_PACKET_INPUT In)
{
	PS_OUTPUT Out;
	float centerDiff = length(In.PosW.zw - float2(0.0f, In.Att2.x)) - abs(In.PosW.w - In.Att2.x);   //	centerDiff = 0; -> straight waves
	float phase = -In.Att.z + (In.PosW.w + centerDiff)*2.0*PI / In.Att.y;
	float3 rippleAdd =	1.0*(1.0 + cos(In.PosW.x*PI)) *(1.0 + cos(In.PosW.y*PI))*In.Att.x // gaussian envelope
						* float3(0, cos(phase), 0);									 	 // ripple function
	Out.Col.xyzw = float4(rippleAdd, 1.0);
	return Out;
}


// 记录顶点位移贴图？
PS_OUTPUT WavePositionPS(PS_POS_INPUT In)
{
	PS_OUTPUT Out; 
	Out.Col = float4(In.PosW.x, In.PosW.y, In.PosW.z, 1.0);
	return Out;
}



//water rendering
PS_OUTPUT DisplayWaterPS(PS_POS_INPUT In)
{
	PS_OUTPUT Out;

	// the derivative of the water displacement texture gives us the water surface normal
	float3 pos = g_waterPosTex.SampleLevel(LinearSampler, In.Tex.xy, 0).xyz + g_waterHeightTex.SampleLevel(LinearSampler, In.Tex.xy, 0).xyz;
	float3 nVec = cross(ddx(pos), -ddy(pos));
	if (dot(nVec, nVec) <= 0)
		nVec = float3(0, -1, 0);
	else
		nVec = normalize(nVec);
	float3 vDir = normalize(In.PosW - g_mWorld[3].xyz);	// view vector
	float3 rDir = vDir - (2.0*dot(vDir, nVec))*nVec;	// reflection vector

	// diffuse/reflective lighting
	float fac = 1.0 - (1.0 - abs(nVec.y) + abs(rDir.y))*(1.0 - abs(nVec.y) + abs(rDir.y));
	Out.Col.xyz = fac*fac*float3(0.1, 0.4, 0.5);

	// add few specular glares
	//float3 glareDir1 = normalize(float3(-1, -0.75, 1));
	//Out.Col.xyz += 100.0*pow(dot(-rDir, glareDir1), 5000);
	const float3 glareDir1 = normalize(float3(-1, -0.75, 1));
	const float3 glareDir2 = normalize(float3(1, -0.75, -1));
	const float3 glareDir3 = normalize(float3(1, -0.75, 1));
	const float3 glareDir4 = normalize(float3(-1, -0.75, -1));
	Out.Col.xyz += 100.0*pow(
	 max(
				dot(-rDir, glareDir4), max(
					dot(-rDir, glareDir3), max(
						dot(-rDir, glareDir2), max(
							0.0, dot(-rDir, glareDir1))
					)
				)
			)
		, 5000);

	// grid overlay
	float floorFactor = 1.0;
	float sth = 0.06;
	float posfac = 1.2*80.0 / SCENE_EXTENT;
	if (frac(posfac*pos.x) < sth)
		floorFactor = 0.5 - 0.5*cos(-PI + 2.0*PI*frac(posfac*pos.x)/ sth);
	if (frac(posfac*pos.z) < sth)
		floorFactor = min(floorFactor, 0.5 - 0.5*cos(-PI + 2.0*PI*frac(posfac*pos.z) / sth));
	Out.Col.xyz *= (0.75+0.25*floorFactor);

	float waterDepth = 1.0 + 0.9*pow(g_waterTerrainTex.SampleLevel(LinearSampler, In.PosW.xz / SCENE_EXTENT + float2(0.5, 0.5), 0).z, 4);
	Out.Col.xyz = waterDepth*Out.Col.xyz;
	/*if (g_waterHeightTex.SampleLevel(LinearSampler, In.Tex.xy, 0).y < 0.01f)
		Out.Col.xyz = float3(1.0, 0.4, 0.5);*/
	Out.Col.w = 1.0;
	return Out;
}



// display the landscape 

PS_OUTPUT DisplayTerrainPS(PS_POS_INPUT In)
{
	PS_OUTPUT Out; 
	if (In.PosW.y < -0.1)
		clip(-1);
	float3 pos = In.PosW;
	Out.Col.xyz = (0.05)*float3(0.05, In.PosW.y, 0.15);
	
	float3 nVec = cross(ddx(pos), -ddy(pos));
	if (dot(nVec, nVec) <= 0)
		nVec = float3(0, -1, 0);
	else
		nVec = normalize(nVec);
	float3 vDir = normalize(In.PosW - g_mWorld[3].xyz);	// view vector
	float3 rDir = vDir - (2.0 * dot(vDir, nVec)) * nVec;	// reflection vector
	// diffuse/reflective lighting
	float3 color = float3(0.5, 0.6, 0.8);
	float fac = 1.0 - (1.0 - abs(nVec.y) + abs(rDir.y)) * (1.0 - abs(nVec.y) + abs(rDir.y));
	Out.Col.xyz += fac * fac * float3(0.1, 0.3, 0.02);
	
	Out.Col.w = 1.0;
	return Out;
}



// downsampling (anti aliasing)
PS_OUTPUT RenderAAPS(PS_INPUT In) 
{ 
    PS_OUTPUT Out;
	const float w[25] = {	0.00296901674395065, 0.013306209891014005, 0.02193823127971504, 0.013306209891014005, 0.00296901674395065, 
							0.013306209891014005, 0.05963429543618023, 0.09832033134884507, 0.05963429543618023, 0.013306209891014005, 
							0.02193823127971504, 0.09832033134884507, 0.16210282163712417, 0.09832033134884507, 0.02193823127971504, 
							0.013306209891014005, 0.05963429543618023, 0.09832033134884507, 0.05963429543618023, 0.013306209891014005, 
							0.00296901674395065, 0.013306209891014005, 0.02193823127971504, 0.013306209891014005, 0.00296901674395065, 
						};
	Out.Col = float4(0,0,0,1.0);
	for (float y=-2; y<2.5; y++)
		for (float x=-2; x<2.5; x++)
		    Out.Col += w[((int)(y)+2)*5+(int)(x)+2]*g_waterHeightTex.SampleLevel(LinearSampler, float2(In.Tex.x,In.Tex.y)+float2(x*g_diffX,y*g_diffY), 0);
	return Out;
}



//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

DepthStencilState soliddepth
{
	DepthEnable = FALSE;
};


RasterizerState state
{
	FillMode = Solid;
	CullMode = None;
	MultisampleEnable = FALSE;
};


RasterizerState AAstate
{
	FillMode = Solid;
	CullMode = None;
	MultisampleEnable = TRUE;
};


RasterizerState stateOutline
{
	FillMode = WireFrame;
	DepthBias = -1.0;
	CullMode = none;
	MultisampleEnable = FALSE;
};


BlendState NoBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
};


BlendState BlendAdd
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
	SrcBlend = ONE;
	DestBlend = ONE;
  	BlendOp = ADD;
	SrcBlendAlpha = ONE;
	DestBlendAlpha = ONE;
	BlendOpAlpha = ADD;
	RenderTargetWriteMask[0] = 0x0F;
};


//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

technique11 RenderSky
{
	pass P1
	{
		SetVertexShader(CompileShader(vs_4_0, SkyVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, SkyPS()));
		SetRasterizerState(state);
		SetBlendState(NoBlending, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xFFFFFFFF);
		SetDepthStencilState(EnableDepth, 0);
	}
}

technique11 RenderBox
{
	pass P1
	{
		SetVertexShader(CompileShader(vs_4_0, CubeVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, CubePS()));
		SetRasterizerState(state);
		SetBlendState(NoBlending, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xFFFFFFFF);
		SetDepthStencilState(EnableDepth, 0);
	}
}

technique11 RasterizeWaveMeshPosition
{
	pass P1
	{
		SetVertexShader(CompileShader(vs_4_0, DisplayWaveMeshVS( )));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, WavePositionPS( )));
		SetRasterizerState(state);
		SetBlendState(NoBlending, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xFFFFFFFF);
		SetDepthStencilState(soliddepth, 0);
	}
}

technique11 AddPacketDisplacement
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, PacketVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, PacketGS( ) ) );
        SetPixelShader( CompileShader( ps_4_0, PacketDisplacementPS( ) ) );
        SetRasterizerState( state );
        SetBlendState( BlendAdd, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( soliddepth, 0 );
    }
}





technique11 DisplayMicroMesh
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, DisplayMicroMeshVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, DisplayMicroMeshGS()) );
        SetPixelShader( CompileShader( ps_4_0, DisplayWaterPS() ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
    }
}


technique11 DisplayTerrain
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, DisplayTerrainVS( ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, DisplayTerrainPS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
    }
}


technique11 RenderAA
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, RenderQuadVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, RenderAAPS() ) );
		SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( soliddepth, 0 );
    }
}