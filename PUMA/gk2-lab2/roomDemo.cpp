#include "roomDemo.h"
#include <array>
#include "mesh.h"
#include "textureGenerator.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

const float WALL_SIZE = 10.0f;
const XMFLOAT4 RoomDemo::LIGHT_POS[1] = { {1.0f, 1.0f, 1.0f, 1.0f} };

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
	m_perlinTexture(m_device.CreateShaderResourceView(L"resources/textures/perlin.jpg")),
	m_smokeTexture(m_device.CreateShaderResourceView(L"resources/textures/smoke.png")),
	m_opacityTexture(m_device.CreateShaderResourceView(L"resources/textures/smokecolors.png")),
	
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
	

	//Meshes
	vector<VertexPositionNormal> vertices;
	vector<unsigned short> indices;
	m_wall = Mesh::Rectangle(m_device, WALL_SIZE);
	m_plate = Mesh::Rectangle(m_device, 2.0f);

	for (int i = 0; i < 6; i++)
		m_puma[i] = Mesh::LoadMesh(m_device, L"resources/robotMesh/mesh" + to_wstring(i + 1) + L".txt");

	m_vbParticles = m_device.CreateVertexBuffer<ParticleVertex>(ParticleSystem::MAX_PARTICLES);

	//World matrix of all objects
	auto temp = XMMatrixTranslation(0.0f, 0.0f, WALL_SIZE / 2);
	auto a = 0.f;
	for (auto i = 0U; i < 4U; ++i, a += XM_PIDIV2)
		XMStoreFloat4x4(&m_wallsMtx[i], temp * XMMatrixRotationY(a) * XMMatrixTranslation(0, WALL_SIZE/2 - 1, 0));
	XMStoreFloat4x4(&m_wallsMtx[4], temp * XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(0, WALL_SIZE / 2 - 1, 0));
	XMStoreFloat4x4(&m_wallsMtx[5], temp * XMMatrixRotationX(-XM_PIDIV2) * XMMatrixTranslation(0, WALL_SIZE / 2 - 1, 0));

	XMStoreFloat4x4(&m_plateMtx, XMMatrixRotationX(XM_PIDIV4 / 2) * XMMatrixRotationY(-XM_PIDIV2) * XMMatrixTranslation(-1.7f, 0.0f, 0.0f));
	
	for (int i=0; i<6; i++)
		XMStoreFloat4x4(&m_pumaMtx[i], XMMatrixIdentity());

	//Calculate and store matrix for mirrored plate
	XMMATRIX plateMtx = XMLoadFloat4x4(&m_plateMtx);
	XMMATRIX mirrorPlateMtx = XMMatrixInverse(nullptr, plateMtx) *
		XMMatrixScaling(1, 1, -1) * plateMtx;
	XMStoreFloat4x4(&m_mirroredPlateMtx, mirrorPlateMtx);

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

	vsCode = m_device.LoadByteCode(L"particleVS.cso");
	psCode = m_device.LoadByteCode(L"particlePS.cso");
	auto gsCode = m_device.LoadByteCode(L"particleGS.cso");
	m_particleVS = m_device.CreateVertexShader(vsCode);
	m_particlePS = m_device.CreatePixelShader(psCode);
	m_particleGS = m_device.CreateGeometryShader(gsCode);
	m_particleLayout = m_device.CreateInputLayout<ParticleVertex>(vsCode);

	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Nie wiem czy używasz do czegoś tych RenderState powyżej, ale ja zainicjalizuję tutaj swoje
	CreateRenderStates();

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

//TODO: refactor
void mini::gk2::RoomDemo::inverse_kinematics(XMFLOAT3 pos, XMFLOAT3 normal, float& a1, float& a2, float& a3, float& a4, float& a5)
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

void RoomDemo::UpdatePuma(float dt)
{
	pumaAngle += XM_2PI * dt / pumaAnimationSpeed;
	XMFLOAT4 pos(0.0f, 0.5f, 0.0f, 1.0f);
	XMFLOAT4 normal(0.0f, 0.0f, -1.0f, 0.0f);

	XMStoreFloat4(&pos, XMVector4Transform(XMLoadFloat4(&pos), XMMatrixRotationZ(pumaAngle) * XMLoadFloat4x4(&m_plateMtx)));
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
	//drwa sheet plate
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4{ 0.25f, 0.25f, 0.25f, 1.0f });
	m_device.context()->OMSetBlendState(m_bsAlpha.get(), nullptr, 0xffffffff);
	DrawMesh(m_plate, m_plateMtx);
	m_device.context()->OMSetBlendState(nullptr, nullptr, 0xffffffff);

	//draw walls
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4{ 0.65f, 0.32f, 0.02f, 1.0f });
	for (int i = 0; i < 6; i++)
		DrawMesh(m_wall, m_wallsMtx[i]);

	//draw puma
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4{ 0.9f, 0.7f, 0.75f, 1.0f });
	for (int i = 0; i < 6; i++)
		DrawMesh(m_puma[i], m_pumaMtx[i]);

}


void RoomDemo::Render()
{
	Base::Render();
	SetShaders(m_phongVS, m_phongPS);
	UpdateBuffer(m_cbProjMtx, m_projMtx);
	UpdateCameraCB();
	DrawMirroredWorld();
	DrawScene();
}

void RoomDemo::DrawMirroredWorld() 
{
	//najpierw zapiszemy do stencila ktora powierzchnia ma odbijać
	m_device.context()->OMSetDepthStencilState(m_dssStencilWrite.get(), 1);
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4{ 0.75f, 0.75f, 0.75f, 1.0f });
	//DrawMesh(m_plate, m_plateMtx);
	UpdateBuffer(m_cbWorldMtx, m_plateMtx);
	m_plate.Render(m_device.context());

	//teraz stencil jest ustawiony na testowanie i narysujemy odbicie na powierzchni
	m_device.context()->OMSetDepthStencilState(m_dssStencilTest.get(), 1);
	m_device.context()->RSSetState(m_rsCCW.get());
	XMMATRIX NewView = XMLoadFloat4x4(&m_mirroredPlateMtx) * m_camera.getViewMatrix();
	UpdateCameraCB(NewView);
	m_device.context()->RSSetState(m_rsCCW.get());
	//draw sheet plate
	//UpdateBuffer(m_cbSurfaceColor, XMFLOAT4{ 1.f, 0.f, 0.f, 1.0f });
	//DrawMesh(m_plate, m_plateMtx);

	//draw walls
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4{ 0.65f, 0.32f, 0.02f, 1.0f });
	for (int i = 0; i < 6; i++)
		DrawMesh(m_wall, m_wallsMtx[i]);

	//draw puma
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4{ 0.9f, 0.7f, 0.75f, 1.0f });
	for (int i = 0; i < 6; i++)
		DrawMesh(m_puma[i], m_pumaMtx[i]);

	
	//ustaw stencila na default
	m_device.context()->OMSetDepthStencilState(nullptr, 1);
	m_device.context()->RSSetState(nullptr);
	//ustaw kamere na defalut
	UpdateCameraCB(m_camera.getViewMatrix());
}

void RoomDemo::CreateRenderStates() 
{
	DepthStencilDescription dssDesc;

	//Setup depth stencil state for writing to stencil buffer
	dssDesc.DepthEnable = false;
	dssDesc.StencilEnable = true;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_dssStencilWrite = m_device.CreateDepthStencilState(dssDesc);

	//Setup depth stencil state for stencil test for 3D objects
	dssDesc.DepthEnable = true;
	dssDesc.StencilEnable = true;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;		//przejdzie jeśli numerek się zgadza
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;	//nie zmieniamy numerka ściany
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	m_dssStencilTest = m_device.CreateDepthStencilState(dssDesc);

	//Setup rasterizer state with ccw front faces
	RasterizerDescription rsDesc;
	rsDesc.FrontCounterClockwise = true;
	m_rsCCW = m_device.CreateRasterizerState(rsDesc);

	BlendDescription bsDesc;
	//Setup alpha blending state
	bsDesc.RenderTarget->BlendEnable = true;
	bsDesc.RenderTarget->SrcBlend = D3D11_BLEND_DEST_COLOR;
	bsDesc.RenderTarget->DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
	bsDesc.RenderTarget->BlendOp = D3D11_BLEND_OP_ADD;
	bsDesc.RenderTarget->SrcBlendAlpha = D3D11_BLEND_ONE;
	bsDesc.RenderTarget->DestBlendAlpha = D3D11_BLEND_ZERO;
	bsDesc.RenderTarget->BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bsDesc.RenderTarget->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	m_bsAlpha = m_device.CreateBlendState(bsDesc);
}