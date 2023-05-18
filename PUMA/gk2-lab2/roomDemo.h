#pragma once
#include "dxApplication.h"
#include "mesh.h"
#include "environmentMapper.h"
#include "particleSystem.h"

namespace mini::gk2
{
	class RoomDemo : public DxApplication
	{
	public:
		using Base = DxApplication;

		explicit RoomDemo(HINSTANCE appInstance);

	protected:
		void Update(const Clock& dt) override;
		void Render() override;

	private:
#pragma region CONSTANTS
		static constexpr float TABLE_H = 1.0f;
		static constexpr float TABLE_TOP_H = 0.1f;
		static constexpr float TABLE_R = 1.5f;
		static constexpr float MAPPER_NEAR = 0.4f;
		static constexpr float MAPPER_FAR = 8.0f;

		float pumaAngle = 0.0f;
		float pumaAnimationSpeed = 4.0f;

		//can't have in-class initializer since XMFLOAT... types' constructors are not constexpr
		static const DirectX::XMFLOAT4 LIGHT_POS[1]; 
#pragma endregion
		dx_ptr<ID3D11Buffer> m_cbWorldMtx, //vertex shader constant buffer slot 0
			m_cbProjMtx,	//vertex shader constant buffer slot 2 & geometry shader constant buffer slot 0
			m_cbTex1Mtx,	//vertex shader constant buffer slot 3
			m_cbTex2Mtx;	//vertex shader constant buffer slot 4
		dx_ptr<ID3D11Buffer> m_cbViewMtx; //vertex shader constant buffer slot 1
		dx_ptr<ID3D11Buffer> m_cbSurfaceColor;	//pixel shader constant buffer slot 0
		dx_ptr<ID3D11Buffer> m_cbLightPos; //pixel shader constant buffer slot 1
		dx_ptr<ID3D11Buffer> m_cbMapMtx; //pixel shader constant buffer slot 2

		Mesh m_puma[6]; //uses m_pumaMtx[6]
		Mesh m_wall; //uses m_wallsMtx[6]
		Mesh m_plate; //uses m_wallsMtx[6]

		dx_ptr<ID3D11Buffer> m_vbParticles;

		DirectX::XMFLOAT4X4 m_projMtx, m_wallsMtx[6], m_pumaMtx[6], m_plateMtx, m_mirroredPlateMtx;

		dx_ptr<ID3D11SamplerState> m_samplerWrap;

		dx_ptr<ID3D11ShaderResourceView> m_particleTexture;
		dx_ptr<ID3D11ShaderResourceView> m_opacityTexture;

		dx_ptr<ID3D11RasterizerState> m_rsCullFront;
		dx_ptr<ID3D11BlendState> m_bsAlpha;
		dx_ptr<ID3D11BlendState> m_bsAlphaParticles;
		dx_ptr<ID3D11DepthStencilState> m_dssNoWrite;

		dx_ptr<ID3D11InputLayout> m_inputlayout, m_particleLayout;

		dx_ptr<ID3D11VertexShader> m_phongVS, m_particleVS;
		dx_ptr<ID3D11GeometryShader> m_particleGS;
		dx_ptr<ID3D11PixelShader> m_phongPS, m_particlePS;


		//Zabawa STENCILEM
		//Depth stencil state used to fill the stencil buffer
		dx_ptr<ID3D11DepthStencilState> m_dssStencilWrite;
		//Depth stencil state used to perform stencil test when drawing mirrored scene
		dx_ptr<ID3D11DepthStencilState> m_dssStencilTest;
		//Rasterizer state used to define front faces as counter-clockwise, used when drawing mirrored scene
		dx_ptr<ID3D11RasterizerState> m_rsCCW;
		dx_ptr<ID3D11DepthStencilState> m_dssNoWriteParticles;
		//Blend state used to draw PLATE faced with alpha blending.
		//dx_ptr<ID3D11BlendState> m_bsAlpha;

		ParticleSystem m_particles;

		void UpdateCameraCB(DirectX::XMMATRIX viewMtx);
		void UpdateCameraCB() { UpdateCameraCB(m_camera.getViewMatrix()); }
		void UpdateParticles(float dt);
		void UpdateParticleEmitter();
		void UpdatePuma(float dt);

		void DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx);
		void DrawParticles();

		void SetWorldMtx(DirectX::XMFLOAT4X4 mtx);
		void SetSurfaceColor(DirectX::XMFLOAT4 color);
		void SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList) { SetTextures(std::move(resList), m_samplerWrap); }

		void inverse_kinematics(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, float& a1, float& a2, float& a3, float& a4, float& a5);

		void DrawScene();
		void DrawMirroredWorld();	//a tu sie ladnie odbicie zrobi
		void CreateRenderStates(); //te wszystkie stencil writy i testy c'nie
	};
}