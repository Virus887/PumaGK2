#include "particleSystem.h"

#include <iterator>

#include "dxDevice.h"
#include "exceptions.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

const D3D11_INPUT_ELEMENT_DESC ParticleVertex::Layout[4] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, //prevPosition
	{ "POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, //  position
	{ "TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, //
	{ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

const XMFLOAT3 ParticleSystem::EMITTER_DIR = XMFLOAT3(0.0f, 1.0f, 0.0f);
const float ParticleSystem::TIME_TO_LIVE = 2.0f;
const float ParticleSystem::EMISSION_RATE = 400.0f;
const float ParticleSystem::MAX_ANGLE = XM_PIDIV2 / 9.0f;
const float ParticleSystem::MIN_VELOCITY = 0.5f;
const float ParticleSystem::MAX_VELOCITY = 5.0f;
const float ParticleSystem::MIN_DIR = -1.0f;
const float ParticleSystem::MAX_DIR = 1.0f;
const float ParticleSystem::MIN_SIZE = 0.002;
const float ParticleSystem::MAX_SIZE = 0.01;
const int ParticleSystem::MAX_PARTICLES = 3000;

ParticleSystem::ParticleSystem(DirectX::XMFLOAT3 emmiterPosition)
	: m_emitterPos(emmiterPosition), m_particlesToCreate(0.0f), m_dirCoordDist(-1.0f, 1.0f),
		m_velDist(MIN_VELOCITY, MAX_VELOCITY), m_random(random_device{}())
{ }

vector<ParticleVertex> ParticleSystem::Update(float dt, DirectX::XMFLOAT4 cameraPosition)
{
	size_t removeCount = 0;
	for (auto& p : m_particles)
	{
		UpdateParticle(p, dt);
		if (p.Vertex.Age >= TIME_TO_LIVE)
			++removeCount;
	}
	m_particles.erase(m_particles.begin(), m_particles.begin() + removeCount);

	m_particlesToCreate += dt * EMISSION_RATE;
	while (m_particlesToCreate >= 1.0f)
	{
		--m_particlesToCreate;
		if (m_particles.size() < MAX_PARTICLES)
			m_particles.push_back(RandomParticle());
	}
	return GetParticleVerts(cameraPosition);
}

XMFLOAT3 ParticleSystem::RandomVelocity()
{
	float x, y, z;

	x = m_dirCoordDist(m_random);
	y = m_dirCoordDist(m_random);
	z = m_dirCoordDist(m_random);
	
	auto a = tan(MAX_ANGLE);

	XMFLOAT3 v((1 + x) * a, 1.0f, (0 + z) * a);

	XMVECTOR velocity = XMLoadFloat3(&v);

	velocity = m_velDist(m_random) * XMVector3Normalize(velocity);
	XMStoreFloat3(&v, velocity);
	return v;
}

Particle ParticleSystem::RandomParticle()
{
	static uniform_real_distribution<float> size(MIN_SIZE, MAX_SIZE);
	Particle p;
	p.Vertex.Age = 0;
	p.Vertex.Size = size(m_random);
	p.Velocities.Velocity = RandomVelocity();
	p.Vertex.Pos = m_emitterPos;

	return p;
}



void ParticleSystem::UpdateParticle(Particle& p, float dt)
{
	XMFLOAT3 acc(0.0f, -9.81f, 0.0f);
	auto gravityAcc = XMLoadFloat3(&acc);

	p.Vertex.PrevPos = p.Vertex.Pos;
	auto pos = XMLoadFloat3(&p.Vertex.Pos);
	auto vel = XMLoadFloat3(&p.Velocities.Velocity);
	p.Vertex.Age += dt;
	XMStoreFloat3(&p.Vertex.Pos, XMVectorAdd(pos, vel * dt));
	XMStoreFloat3(&p.Velocities.Velocity, XMVectorAdd(vel, gravityAcc * dt));
}

struct ParticleData {
	ParticleVertex vertex;
	float distance;
};

vector<ParticleVertex> ParticleSystem::GetParticleVerts(DirectX::XMFLOAT4 cameraPosition)
{
	XMFLOAT4 cameraTarget(0.0f, 0.0f, 0.0f, 1.0f);

	vector<ParticleVertex> vertices;
	vertices.reserve(m_particles.size());
	transform(m_particles.begin(), m_particles.end(), std::back_inserter(vertices), [](const Particle& p) { return p.Vertex; });
	XMVECTOR camPos = XMLoadFloat4(&cameraPosition);
	XMVECTOR camDir = XMVectorSubtract(XMLoadFloat4(&cameraTarget), camPos);
	sort(vertices.begin(), vertices.end(), [camPos, camDir](auto& p1, auto& p2)
		{
			auto p1Pos = XMLoadFloat3(&(p1.Pos));
			p1Pos.m128_f32[3] = 1.0f;
			auto p2Pos = XMLoadFloat3(&(p2.Pos));
			p2Pos.m128_f32[3] = 1.0f;
			auto d1 = XMVector3Dot(p1Pos - camPos, camDir).m128_f32[0];
			auto d2 = XMVector3Dot(p2Pos - camPos, camDir).m128_f32[0];
			return d1 > d2;
		});

	return vertices;
}