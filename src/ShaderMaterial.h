#ifndef _SHADER_MATERIAL_H_
#define _SHADER_MATERIAL_H_

#include "RenderTargetHolder.h"

/**	Класс для шейдерных материалов.
*/
class ShaderMaterial
{
	/**	Класс для хранения информации об используемых текстурах.
		Класс может хранить указатель либо на таргет, либо на текстуру, либо на VolumeTexture.
	*/
	class Texture
	{
		RenderTargetHolder* _renderTargetHolder;
		Render::Texture* _texture;
		Render::VolumeTexture* _volumeTexture;
		int _channel;
		int _stageOp;
	public:
		Texture();
		Texture(RenderTargetHolder* rtHolder, int channel, int stageOp);
		Texture(Render::Texture* texture, int channel, int stageOp);
		Texture(Render::VolumeTexture* texture, int channel, int stageOp);
		void Set(RenderTargetHolder* rtHolder, int channel, int stageOp);
		void Set(Render::Texture* texture, int channel, int stageOp);
		void Set(Render::VolumeTexture* texture, int channel, int stageOp);
		void Bind() const;
	};

public:

	class ShaderParameters
	{
	public:
		class Parameter
		{
			std::vector<float> _value;
		public:
			void SetPS(const std::string& paramName, Render::ShaderProgram* shaderProgram) const;
			void SetVS(const std::string& paramName, Render::ShaderProgram* shaderProgram) const;
			void SetValue(float f);
			void SetValue(const math::Vector4& v);
			void SetValue(const math::Matrix4& m);
			void SetValue(const float* fp, size_t size);
		};
	
		struct AutoParameter
		{
			std::string _type;
			Parameter _parameter;
	
			void Update();
			void SetPS(const std::string& paramName, Render::ShaderProgram* shaderProgram) const;
			void SetVS(const std::string& paramName, Render::ShaderProgram* shaderProgram) const; 
		};
	
		void setParam(const std::string& name, float f);
		void setParam(const std::string& name, const math::Vector4& v);
		void setParam(const std::string& name, const math::Matrix4& m);
		void setParam(const std::string& name, const float* fp, size_t size);
		void setParamAuto(const std::string& name, const std::string& paramType);
	
		void SetVS(Render::ShaderProgram* shaderProgram);
		void SetPS(Render::ShaderProgram* shaderProgram);
	
	private:

		void UpdateAutoParameters();
		std::map<std::string, Parameter> _parameterMap;
		std::map<std::string, AutoParameter> _autoParameterMap;
	};

private:

	std::vector<Texture> _textures;
	Render::ShaderProgram* _shaderProgram;

	ShaderParameters _vsParameters;
	ShaderParameters _psParameters;


public:
	ShaderMaterial();
	~ShaderMaterial();

	void SetTexture(RenderTargetHolder* rtHolder, int channel, int stageOp);
	void SetTexture(Render::Texture* texture, int channel, int stageOp);
	void SetTexture(Render::VolumeTexture* texture, int channel, int stageOp);
	void RemoveTexture(int channel);
	void ClearTextures();

	void SetShaderProgram(Render::ShaderProgram* shader);
	Render::ShaderProgram* GetShaderProgram() const;

	void Bind();
	void Unbind();

	ShaderParameters& GetVSParameters();
	ShaderParameters& GetPSParameters();
};

#endif //_SHADER_MATERIAL_H_