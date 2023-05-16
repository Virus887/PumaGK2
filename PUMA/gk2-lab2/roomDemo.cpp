#include "roomDemo.h"
#include <array>
#include "mesh.h"
#include "textureGenerator.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

const XMFLOAT3 RoomDemo::TEAPOT_POS{ -1.3f, -0.74f, -0.6f };
const XMFLOAT4 RoomDemo::TABLE_POS{ 0.5f, -0.96f, 0.5f, 1.0f };
const XMFLOAT4 RoomDemo::LIGHT_POS[2] = { {1.0f, 1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f, 1.0f} };

RoomDemo::RoomDemo(HINSTANCE appInstance)
	: DxApplication(appInstance, 1280, 720, L"PUMA Bis Jedliczko"), 
	//Constant Buffers
	m_cbWorldMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbProjMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()), m_cbTex1Mtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbTex2Mtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbViewMtx(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbLightPos(m_device.CreateConstantBuffer<XMFLOAT4, 2>()),
	//Textures
	m_wallTexture(m_device.CreateShaderResourceView(L"resources/textures/brick_wall.jpg")),
	m_posterTexture(m_device.CreateShaderResourceView(L"resources/textures/lautrec_divan.jpg")),
	m_perlinTexture(m_device.CreateShaderResourceView(L"resources/textures/perlin.jpg")),
	m_smokeTexture(m_device.CreateShaderResourceView(L"resources/textures/smoke.png")),
	m_opacityTexture(m_device.CreateShaderResourceView(L"resources/textures/smokecolors.png")),
	//EnvMapper
	m_envMapper{ m_device, MAPPER_NEAR, MAPPER_FAR, TEAPOT_POS },
	//Particles
	m_particles{ {-1.3f, -0.6f, -0.14f} }
{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	UpdateBuffer(m_cbProjMtx, m_projMtx);
	UpdateCameraCB();

	//Sampler States
	SamplerDescription sd;
	// TODO : 0.01 Set to proper addressing (wrap) and filtering (16x anisotropic) modes of the sampler
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; sd.Filter = D3D11_FILTER_ANISOTROPIC; sd.MaxAnisotropy = 16;
	m_samplerWrap = m_device.CreateSamplerState(sd);
	// TODO : 1.06 Initialize second sampler state
	sd.AddressU = sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 0;
	
	// TODO : 1.10 Moddify MipLODBias field for the second sampler
	sd.MipLODBias = 1;
	m_sampler2 = m_device.CreateSamplerState(sd);

	//Wood texture
	constexpr auto woodTexWidth = 64U;
	constexpr auto woodTexHeight = 64U;
	constexpr auto woodTexBpp = 4U;
	constexpr auto woodTexStride = woodTexWidth*woodTexBpp;
	constexpr auto woodTexSize = woodTexStride*woodTexHeight;

	auto texDesc = Texture2DDescription(woodTexWidth, woodTexHeight);
	texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	auto woodTex = m_device.CreateTexture(texDesc);
	m_woodTexture = m_device.CreateShaderResourceView(woodTex);

	array<BYTE, woodTexSize> data;
	auto d = data.data();
	TextureGenerator txGen(6, 0.35f);
	for (auto i = 0; i < woodTexHeight; ++i)
	{
		auto x = i / static_cast<float>(woodTexHeight);
		for (auto j = 0; j < woodTexWidth; ++j)
		{
			auto y = j / static_cast<float>(woodTexWidth);
			auto c = txGen.Wood(x, y);
			auto ic = static_cast<BYTE>(c * 239);
			*(d++) = ic;
			ic = static_cast<BYTE>(c * 200);
			*(d++) = ic;
			ic = static_cast<BYTE>(c * 139);
			*(d++) = ic;
			*(d++) = 255;
		}
	}
	m_device.context()->UpdateSubresource(woodTex.get(), 0, nullptr, data.data(), woodTexStride, woodTexSize);
	m_device.context()->GenerateMips(m_woodTexture.get());

	//Meshes
	vector<VertexPositionNormal> vertices;
	vector<unsigned short> indices;
	m_wall = Mesh::Rectangle(m_device, 4.0f);
	m_plate = Mesh::Rectangle(m_device, 2.0f);

	for (int i = 0; i < 6; i++)
		m_puma[i] = Mesh::LoadMesh(m_device, L"resources/robotMesh/mesh" + to_wstring(i + 1) + L".txt");

	m_vbParticles = m_device.CreateVertexBuffer<ParticleVertex>(ParticleSystem::MAX_PARTICLES);

	//World matrix of all objects
	auto temp = XMMatrixTranslation(0.0f, 0.0f, 2.0f);
	auto a = 0.f;
	for (auto i = 0U; i < 4U; ++i, a += XM_PIDIV2)
		XMStoreFloat4x4(&m_wallsMtx[i], temp * XMMatrixRotationY(a));
	XMStoreFloat4x4(&m_wallsMtx[4], temp * XMMatrixRotationX(XM_PIDIV2));
	XMStoreFloat4x4(&m_wallsMtx[5], temp * XMMatrixRotationX(-XM_PIDIV2));
	XMStoreFloat4x4(&m_teapotMtx, XMMatrixTranslation(0.0f, -2.3f, 0.f) * XMMatrixScaling(0.1f, 0.1f, 0.1f) *
		XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(-1.3f, -0.74f, -0.6f));
	
	XMStoreFloat4x4(&m_sphereMtx, XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(TEAPOT_POS.x, TEAPOT_POS.y, TEAPOT_POS.z));
	XMStoreFloat4x4(&m_boxMtx, XMMatrixTranslation(-1.4f, -1.46f, -0.6f));
	XMStoreFloat4x4(&m_plateMtx, XMMatrixRotationX(XM_PIDIV4 / 2) * XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(-1.7f, 0.0f, 0.0f));
	
	for (int i=0; i<6; i++)
		XMStoreFloat4x4(&m_pumaMtx[i], XMMatrixIdentity());

	XMStoreFloat4x4(&m_chairMtx, XMMatrixRotationY(XM_PI + XM_PI / 9 ) *
		XMMatrixTranslation(-0.1f, -1.06f, -1.3f));
	XMStoreFloat4x4(&m_monitorMtx, XMMatrixRotationY(XM_PIDIV4) *
		XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y + 0.42f, TABLE_POS.z));
	a = XM_PIDIV4;
	for (auto i = 0U; i < 4U; ++i, a += XM_PIDIV2)
		XMStoreFloat4x4(&m_tableLegsMtx[i], XMMatrixTranslation(0.0f, 0.0f, TABLE_R - 0.35f) * XMMatrixRotationY(a) *
			XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y - (TABLE_H + TABLE_TOP_H) / 2, TABLE_POS.z));
	XMStoreFloat4x4(&m_tableSideMtx, XMMatrixRotationY(XM_PIDIV4 / 4) *
		XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y - TABLE_TOP_H / 2, TABLE_POS.z));
	XMStoreFloat4x4(&m_tableTopMtx, XMMatrixRotationY(XM_PIDIV4 / 4) *
		XMMatrixTranslation(TABLE_POS.x, TABLE_POS.y, TABLE_POS.z));

	//Constant buffers content
	UpdateBuffer(m_cbLightPos, LIGHT_POS);
	XMFLOAT4X4 tempMtx;

	// TODO : 1.08 Calculate correct transformation matrix for the poster texture
	
	XMStoreFloat4x4(&tempMtx, XMMatrixTranslation(0.8f, 0.0f, 0.0f)* XMMatrixRotationZ(- XM_PI/9)* XMMatrixScaling(1.0f, -0.75f, 1.0f)*XMMatrixTranslation(0.5f, 0.5f, 0.0f));
	//XMStoreFloat4x4(&tempMtx, XMMatrixIdentity());
	
	UpdateBuffer(m_cbTex2Mtx, tempMtx);

	//Render states
	RasterizerDescription rsDesc;
	rsDesc.CullMode = D3D11_CULL_FRONT;
	m_rsCullFront = m_device.CreateRasterizerState(rsDesc);

	m_bsAlpha = m_device.CreateBlendState(BlendDescription::AlphaBlendDescription());
	DepthStencilDescription dssDesc;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);

	auto vsCode = m_device.LoadByteCode(L"phongVS.cso");
	auto psCode = m_device.LoadByteCode(L"phongPS.cso");
	m_phongVS = m_device.CreateVertexShader(vsCode);
	m_phongPS = m_device.CreatePixelShader(psCode);
	m_inputlayout = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	vsCode = m_device.LoadByteCode(L"texturedVS.cso");
	psCode = m_device.LoadByteCode(L"texturedPS.cso");
	m_textureVS = m_device.CreateVertexShader(vsCode);
	m_texturePS = m_device.CreatePixelShader(psCode);
	psCode = m_device.LoadByteCode(L"colorTexPS.cso");
	m_colorTexPS = m_device.CreatePixelShader(psCode);

	vsCode = m_device.LoadByteCode(L"multiTexVS.cso");
	psCode = m_device.LoadByteCode(L"multiTexPS.cso");
	m_multiTexVS = m_device.CreateVertexShader(vsCode);
	m_multiTexPS = m_device.CreatePixelShader(psCode);

	vsCode = m_device.LoadByteCode(L"particleVS.cso");
	psCode = m_device.LoadByteCode(L"particlePS.cso");
	auto gsCode = m_device.LoadByteCode(L"particleGS.cso");
	m_particleVS = m_device.CreateVertexShader(vsCode);
	m_particlePS = m_device.CreatePixelShader(psCode);
	m_particleGS = m_device.CreateGeometryShader(gsCode);
	m_particleLayout = m_device.CreateInputLayout<ParticleVertex>(vsCode);

	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UpdatePuma(0.0f);

	//We have to make sure all shaders use constant buffers in the same slots!
	//Not all slots will be use by each shader
	ID3D11Buffer* vsb[] = { m_cbWorldMtx.get(),  m_cbViewMtx.get(), m_cbProjMtx.get(), m_cbTex1Mtx.get(), m_cbTex2Mtx.get() };
	m_device.context()->VSSetConstantBuffers(0, 5, vsb); //Vertex Shaders - 0: worldMtx, 1: viewMtx,invViewMtx, 2: projMtx, 3: tex1Mtx, 4: tex2Mtx
	m_device.context()->GSSetConstantBuffers(0, 1, vsb + 2); //Geometry Shaders - 0: projMtx
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLightPos.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb); //Pixel Shaders - 0: surfaceColor, 1: lightPos[2]
}

void RoomDemo::UpdateCameraCB(XMMATRIX viewMtx)
{
	XMVECTOR det;
	XMMATRIX invViewMtx = XMMatrixInverse(&det, viewMtx);
	XMFLOAT4X4 view[2];
	XMStoreFloat4x4(view, viewMtx);
	XMStoreFloat4x4(view + 1, invViewMtx);
	UpdateBuffer(m_cbViewMtx, view);
}

void mini::gk2::RoomDemo::UpdateParticles(float dt)
{
	// TODO : 1.31 update particle system and copy vertex data to the buffer
	auto particles = m_particles.Update(dt, m_camera.getCameraPosition());
	UpdateBuffer(m_vbParticles, particles);
}

void RoomDemo::Update(const Clock& c)
{
	double dt = c.getFrameTime();
	HandleCameraInput(dt);
	UpdatePuma(static_cast<float>(dt));
	UpdateParticles(dt);
}

void RoomDemo::SetWorldMtx(DirectX::XMFLOAT4X4 mtx)
{
	UpdateBuffer(m_cbWorldMtx, mtx);
}

void RoomDemo::SetSurfaceColor(DirectX::XMFLOAT4 color)
{
	UpdateBuffer(m_cbSurfaceColor, color);
}

void mini::gk2::RoomDemo::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps)
{
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
}

void mini::gk2::RoomDemo::SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->PSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->PSSetSamplers(0, 1, &s_ptr);
}

void RoomDemo::DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

void RoomDemo::DrawParticles()
{
	if (m_particles.particlesCount() == 0)
		return;
	//Set input layout, primitive topology, shaders, vertex buffer, and draw particles
	SetTextures({ m_smokeTexture.get(), m_opacityTexture.get() });
	m_device.context()->IASetInputLayout(m_particleLayout.get());
	SetShaders(m_particleVS, m_particlePS);
	m_device.context()->GSSetShader(m_particleGS.get(), nullptr, 0);
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	unsigned int stride = sizeof(ParticleVertex);
	unsigned int offset = 0;
	auto vb = m_vbParticles.get();
	m_device.context()->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	m_device.context()->Draw(m_particles.particlesCount(), 0);

	//Reset layout, primitive topology and geometry shader
	m_device.context()->GSSetShader(nullptr, nullptr, 0);
	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void RoomDemo::DrawWalls()
{
	//draw ceiling
	XMFLOAT4X4 texMtx;
	SetSurfaceColor(XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));

	// TODO : 0.02 calculate texture tranformation matrix for walls/ceiling/floor that transforms rectangle [-2,-2]x[2,2] into [0,0]x[1,1]
	auto Matrix = DirectX::XMMatrixScaling(0.25, 0.25, 0.25) * DirectX::XMMatrixTranslation(0.5, 0.5, 0.5);
	XMStoreFloat4x4(&texMtx, Matrix);
	//XMStoreFloat4x4(&texMtx, XMMatrixIdentity());
	//XMStoreFloat4x4(&texMtx, XMMatrixIdentity());
	UpdateBuffer(m_cbTex1Mtx, texMtx);


	// TODO : 0.03 set shaders to m_textureVS, m_texturePS
	//SetShaders(m_textureVS, m_texturePS);
	// TODO : 0.11 draw ceiling with colorTexture pixel shaders instead
	//SetShaders(m_phongVS, m_phongPS);
	SetShaders(m_textureVS, m_colorTexPS);

	// TODO : 0.04 set ceiling the texture to perlin noise
	SetTextures({m_perlinTexture.get()});

	DrawMesh(m_wall, m_wallsMtx[5]);

	//draw back wall
	SetSurfaceColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	
	// TODO : 1.04 Draw back wall with muliti-texture shaders, setting textures to wall and poster.
	SetShaders(m_multiTexVS, m_multiTexPS);
	SetTextures({m_wallTexture.get(),m_posterTexture.get()},m_sampler2);
	// TODO : 1.07 Pass the new sampler state along with wall and poster textures

	

	// TODO : 0.12 draw back walls with regular texture shaders
	//SetTextures({m})
	
	//SetShaders(m_textureVS, m_texturePS);
	DrawMesh(m_wall, m_wallsMtx[0]);
	// TODO : 0.05 set the walls' texture to brick wall 
	SetTextures({ m_wallTexture.get() });
	SetShaders(m_textureVS, m_texturePS);
	//draw remainting walls
	// TODO : 0.13 draw remaining walls with regular texture shaders
	for (auto i = 1; i < 4; ++i)
		DrawMesh(m_wall, m_wallsMtx[i]);

	//draw floor
	// TODO : 0.09 Change texture matrix to stretch floor texture 8 times along y, i.e. transform rectangle [-2,-2]x[2,2] into [0,0]x[1,16]. Run the program again to see the difference.
	Matrix = DirectX::XMMatrixScaling(0.25, 4, 0.25) * DirectX::XMMatrixTranslation(0.5, 8.0, 0.5);
	XMStoreFloat4x4(&texMtx, Matrix);
	UpdateBuffer(m_cbTex1Mtx, texMtx);
	// TODO : 0.06 set floor texture to wood
	SetTextures({ m_woodTexture.get() });

	DrawMesh(m_wall, m_wallsMtx[4]);
}

void mini::gk2::RoomDemo::inverse_kinematics(XMFLOAT3 pos, XMFLOAT3 normal, float& a1, float& a2,
	float& a3, float& a4, float& a5)
{
	float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;
	XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));
	XMFLOAT3 pos1;
	XMStoreFloat3(&pos1, XMLoadFloat3(&pos) + XMLoadFloat3(&normal) * l3);
	float e = sqrtf(pos1.z * pos1.z + pos1.x * pos1.x - dz * dz);
	a1 = atan2(pos1.z, -pos1.x) + atan2(dz, e);
	XMFLOAT3 pos2(e, pos1.y - dy, .0f);
	a3 = -acosf(min(1.0f, (pos2.x * pos2.x + pos2.y * pos2.y - l1 * l1 - l2 * l2)
		/ (2.0f * l1 * l2)));
	float k = l1 + l2 * cosf(a3), l = l2 * sinf(a3);
	a2 = -atan2(pos2.y, sqrtf(pos2.x * pos2.x + pos2.z * pos2.z)) - atan2(l, k);
	XMFLOAT3 normal1;
	XMStoreFloat3(&normal1, XMVector3Transform(XMLoadFloat3(&normal), XMMatrixRotationY(-a1) * XMMatrixRotationZ(-(a2 + a3))));

	a5 = acosf(normal1.x);
	a4 = atan2(normal1.z, normal1.y);
}



float angle = 0;
float period = 6;

void RoomDemo::UpdatePuma(float dt)
{
	angle += dt / period * XM_2PI;
	XMFLOAT4 pos(0.0f, 0.5f, 0.0f, 1.0f);
	XMFLOAT4 normal(0.0f, 0.0f, -1.0f, 0.0f);
	XMStoreFloat4(&pos, XMVector4Transform(XMLoadFloat4(&pos), XMMatrixRotationZ(angle) * XMLoadFloat4x4(&m_plateMtx)));
	XMStoreFloat4(&normal, XMVector4Transform(XMLoadFloat4(&normal), XMLoadFloat4x4(&m_plateMtx)));

	float a1, a2, a3, a4, a5;
	inverse_kinematics({ pos.x, pos.y, pos.z }, { normal.x, normal.y, normal.z }, a1, a2, a3, a4, a5);

	XMStoreFloat4x4(&m_pumaMtx[1], XMMatrixRotationY(a1));
	XMStoreFloat4x4(&m_pumaMtx[2], XMMatrixTranslation(0.0f, -0.27f, 0.0f) * XMMatrixRotationZ(a2) * XMMatrixTranslation(0.0f, 0.27f, 0.0f) * XMLoadFloat4x4(&m_pumaMtx[1]));
	XMStoreFloat4x4(&m_pumaMtx[3], XMMatrixTranslation(0.91f, -0.27f, 0.0f) * XMMatrixRotationZ(a3) * XMMatrixTranslation(-0.91f, 0.27f, 0.0f) * XMLoadFloat4x4(&m_pumaMtx[2]));
	XMStoreFloat4x4(&m_pumaMtx[4], XMMatrixTranslation(0.0f, -0.27f, 0.26f) * XMMatrixRotationX(a4) * XMMatrixTranslation(0.0f, 0.27f, -0.26f) * XMLoadFloat4x4(&m_pumaMtx[3]));
	XMStoreFloat4x4(&m_pumaMtx[5], XMMatrixTranslation(1.72f, -0.27f, 0.0f) * XMMatrixRotationZ(a5) * XMMatrixTranslation(-1.72f, 0.27f, 0.0f) * XMLoadFloat4x4(&m_pumaMtx[4]));
}


void RoomDemo::DrawScene()
{
	DrawWalls();
	SetShaders(m_phongVS, m_phongPS);

	for (int i = 0; i < 6; i++)
		DrawMesh(m_puma[i], m_pumaMtx[i]);

	DrawMesh(m_plate, m_plateMtx);
}


void RoomDemo::Render()
{
	Base::Render();
	// TODO : 1.20 Change projection matrix to for drawing in environment cube
	UpdateBuffer(m_cbProjMtx, m_envMapper.FaceProjMtx());
	// TODO : 1.21 Set evnMapper render target
	m_envMapper.SetTarget(m_device.context());
	// TODO : 1.22 For each cube face: clear envMapper render target, update camera constant buffer, draw the scene and copy the result to cubemap
	
	for (int i = 0; i < 6; i++) 
	{
		m_envMapper.ClearTarget(m_device.context());
		UpdateCameraCB(m_envMapper.FaceViewMtx(D3D11_TEXTURECUBE_FACE(i)));
		DrawScene();
		m_envMapper.SaveFace(m_device.context(), D3D11_TEXTURECUBE_FACE(i));
	}


	//wymyslone
	//auto s = m_window.getClientSize();
	//auto ar = static_cast<float>(s.cx) / s.cy;
	//XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));


	ResetRenderTarget();
	UpdateBuffer(m_cbProjMtx, m_projMtx);
	UpdateCameraCB();
	DrawScene();
}