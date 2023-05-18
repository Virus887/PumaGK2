#pragma once
#include <DirectXMath.h>
#include <vector>
#include <random>
#include <d3d11.h>

namespace mini
{
	namespace gk2
	{
		struct ParticleVertex
		{
			DirectX::XMFLOAT3 PrevPos;
			DirectX::XMFLOAT3 Pos;
			float Age;
			float Size;
			static const D3D11_INPUT_ELEMENT_DESC Layout[4];

			ParticleVertex() : PrevPos(0.0f, 0.0f, 0.0f), Pos(0.0f, 0.0f, 0.0f), Age(0.0f), Size(0.0f) { }
		};

		struct ParticleVelocities
		{
			DirectX::XMFLOAT3 Velocity;

			ParticleVelocities() : Velocity(0.0f, 0.0f, 0.0f) { }
		};

		struct Particle
		{
			ParticleVertex Vertex;
			ParticleVelocities Velocities;
		};

		class ParticleSystem
		{
		public:
			ParticleSystem() = default;

			ParticleSystem(ParticleSystem&& other) = default;

			ParticleSystem(DirectX::XMFLOAT3 emmiterPosition);

			ParticleSystem& operator=(ParticleSystem&& other) = default;

			std::vector<ParticleVertex> Update(float dt, DirectX::XMFLOAT4 cameraPosition);

			size_t particlesCount() const { return m_particles.size(); }

			static const int MAX_PARTICLES;		//maximal number of particles in the system
			static const float TIME_TO_LIVE;	//time of particle's life in seconds

			void SetEmitterPos(DirectX::XMFLOAT3 pos) { m_emitterPos = pos; };
			void SetEmitterDir(DirectX::XMFLOAT3 dir) { m_emitterDir = dir; };


		private:
			static const DirectX::XMFLOAT3 EMITTER_DIR;	//mean direction of particles' velocity
			static const float EMISSION_RATE;	//number of particles to be born per second
			static const float MAX_ANGLE;		//maximal angle declination from mean direction
			static const float MIN_SIZE;	//minimal particle size
			static const float MAX_SIZE;	//maximal particle size

			static const float MIN_VELOCITY, MAX_VELOCITY;

			static const float MIN_DIR, MAX_DIR;

			DirectX::XMFLOAT3 m_emitterPos;
			DirectX::XMFLOAT3 m_emitterDir;
			float m_particlesToCreate;

			std::vector<Particle> m_particles;

			std::default_random_engine m_random;
			std::uniform_real_distribution<float> m_dirCoordDist;
			std::uniform_real_distribution<float> m_velDist;

			DirectX::XMFLOAT3 RandomVelocity();
			//DirectX::XMFLOAT3 RandomDirection();
			Particle RandomParticle();
			static void UpdateParticle(Particle& p, float dt);
			std::vector<ParticleVertex> GetParticleVerts(DirectX::XMFLOAT4 cameraPosition);
		};
	}
}