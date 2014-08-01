#include "stdafx.h"
#include "ShaderMaterial.h"
#include "Render/VolumeTexture.h"

ShaderMaterial::Texture::Texture():
	_renderTargetHolder(0),
	_texture(0),
	_volumeTexture(0),
	_channel(0),
	_stageOp(0)
{

}

ShaderMaterial::Texture::Texture(RenderTargetHolder* rtHolder, int channel, int stageOp):
	_renderTargetHolder(rtHolder),
	_texture(0),
	_volumeTexture(0),
	_channel(channel),
	_stageOp(stageOp)
{

}

ShaderMaterial::Texture::Texture(Render::Texture* texture, int channel, int stageOp):
	_renderTargetHolder(0),
	_texture(texture),
	_volumeTexture(0),
	_channel(channel),
	_stageOp(stageOp)
{

}

ShaderMaterial::Texture::Texture(Render::VolumeTexture* texture, int channel, int stageOp):
	_renderTargetHolder(0),
	_texture(0),
	_volumeTexture(texture),
	_channel(channel),
	_stageOp(stageOp)
{

}

void ShaderMaterial::Texture::Set(RenderTargetHolder* rtHolder, int channel, int stageOp)
{
	_renderTargetHolder = rtHolder;
	_texture = 0;
	_volumeTexture = 0;
	_channel = channel;
	_stageOp = stageOp;
}

void ShaderMaterial::Texture::Set(Render::Texture* texture, int channel, int stageOp)
{
	_renderTargetHolder = 0;
	_texture = texture;
	_volumeTexture = 0;
	_channel = channel;
	_stageOp = stageOp;
}

void ShaderMaterial::Texture::Set(Render::VolumeTexture* texture, int channel, int stageOp)
{
	_renderTargetHolder = 0;
	_texture = 0;
	_volumeTexture = texture;
	_channel = channel;
	_stageOp = stageOp;
}

void ShaderMaterial::Texture::Bind() const
{
	if(_texture) _texture->Bind(_channel, _stageOp);
	if(_renderTargetHolder) _renderTargetHolder->Bind(_channel, _stageOp);
	if(_volumeTexture) _volumeTexture->Bind(_channel, _stageOp);
}


void ShaderMaterial::ShaderParameters::Parameter::SetPS(
	const std::string& paramName, Render::ShaderProgram* shaderProgram) const
{
	if(_value.empty()) return;
	shaderProgram->setPSParam(paramName, &_value[0], _value.size());
}

void ShaderMaterial::ShaderParameters::Parameter::SetVS(
	const std::string& paramName, Render::ShaderProgram* shaderProgram) const
{
	if(_value.empty()) return;
	shaderProgram->setVSParam(paramName, &_value[0], _value.size());
}

void ShaderMaterial::ShaderParameters::Parameter::SetValue(
	float f)
{
	SetValue(math::Vector4(f, 0, 0, 0));
}

void ShaderMaterial::ShaderParameters::Parameter::SetValue(
	const math::Vector4& v)
{
	_value.resize(4);
	for(int i = 0; i != 4; ++i)
		_value[i] = v.v[i];
}

void ShaderMaterial::ShaderParameters::Parameter::SetValue(
	const math::Matrix4& m)
{
	_value.resize(16);
	for(int i = 0; i != 16; ++i)
		_value[i] = m.v[i];
}

void ShaderMaterial::ShaderParameters::Parameter::SetValue(const float* fp, size_t size)
{
	_value.resize(size);
	for(int i = 0; i != size; ++i)
		_value[i] = fp[i];
}


void ShaderMaterial::ShaderParameters::AutoParameter::Update()
{
	if(_type == "ModelViewProjectionMatrix")
		_parameter.SetValue(Render::device.GetModelViewProjectionMatrix());
}

void ShaderMaterial::ShaderParameters::AutoParameter::SetPS(
	const std::string& paramName, Render::ShaderProgram* shaderProgram) const
{
	_parameter.SetPS(paramName, shaderProgram);
}

void ShaderMaterial::ShaderParameters::AutoParameter::SetVS(
	const std::string& paramName, Render::ShaderProgram* shaderProgram) const
{
	_parameter.SetVS(paramName, shaderProgram);
}


void ShaderMaterial::ShaderParameters::setParam(
	const std::string& name, float f)
{
	_parameterMap[name].SetValue(f);
}

void ShaderMaterial::ShaderParameters::setParam(
	const std::string& name, const math::Vector4& v)
{
	_parameterMap[name].SetValue(v);
}

void ShaderMaterial::ShaderParameters::setParam(
	const std::string& name, const math::Matrix4& m)
{
	_parameterMap[name].SetValue(m);
}

void ShaderMaterial::ShaderParameters::setParam(
	const std::string& name, const float* fp, size_t size)
{
	_parameterMap[name].SetValue(fp, size);
}

void ShaderMaterial::ShaderParameters::setParamAuto(
	const std::string& name, const std::string& paramType)
{
	_autoParameterMap[name]._type = paramType;
}

void ShaderMaterial::ShaderParameters::SetVS(
	Render::ShaderProgram* shaderProgram)
{
	UpdateAutoParameters();

	for(std::map<std::string, AutoParameter>::iterator it = _autoParameterMap.begin();
		it != _autoParameterMap.end();
		++it)
	{
		it->second._parameter.SetVS(it->first, shaderProgram);
	}

	for(std::map<std::string, Parameter>::iterator it = _parameterMap.begin();
		it != _parameterMap.end();
		++it)
	{
		it->second.SetVS(it->first, shaderProgram);
	}
}

void ShaderMaterial::ShaderParameters::SetPS(
	Render::ShaderProgram* shaderProgram)
{
	UpdateAutoParameters();

	for(std::map<std::string, AutoParameter>::iterator it = _autoParameterMap.begin();
		it != _autoParameterMap.end();
		++it)
	{
		it->second._parameter.SetPS(it->first, shaderProgram);
	}

	for(std::map<std::string, Parameter>::iterator it = _parameterMap.begin();
		it != _parameterMap.end();
		++it)
	{
		it->second.SetPS(it->first, shaderProgram);
	}
}

void ShaderMaterial::ShaderParameters::UpdateAutoParameters()
{
	for(std::map<std::string, AutoParameter>::iterator it = _autoParameterMap.begin();
		it != _autoParameterMap.end();
		++it)
	{
		it->second.Update();
	}
}

		
ShaderMaterial::ShaderMaterial():
	_shaderProgram(0)
{
	ClearTextures();
}

ShaderMaterial::~ShaderMaterial()
{

}

void ShaderMaterial::SetTexture(RenderTargetHolder* rtHolder, int channel, int stageOp)
{
	if(channel < (int)_textures.size())
		_textures[channel] = Texture(rtHolder, channel, stageOp);
}

void ShaderMaterial::SetTexture(Render::Texture* texture, int channel, int stageOp)
{
	if(channel < (int)_textures.size())
		_textures[channel] = Texture(texture, channel, stageOp);
}

void ShaderMaterial::SetTexture(Render::VolumeTexture* texture, int channel, int stageOp)
{
	if(channel < (int)_textures.size())
		_textures[channel] = Texture(texture, channel, stageOp);
}

void ShaderMaterial::RemoveTexture(int channel)
{
	if(channel < static_cast<int>(_textures.size()))
		_textures[channel] = Texture();
}

void ShaderMaterial::ClearTextures()
{
	_textures.clear();
	_textures.resize(Render::device.GetMaxTextureUnits());
}

void ShaderMaterial::SetShaderProgram(Render::ShaderProgram* shader)
{
	_shaderProgram = shader;
}

Render::ShaderProgram* ShaderMaterial::GetShaderProgram() const
{
	return _shaderProgram;
}

void ShaderMaterial::Bind()
{
	if(_shaderProgram)
	{
		_shaderProgram->Bind();

		_vsParameters.SetVS(_shaderProgram);
		_psParameters.SetPS(_shaderProgram);
	}

	for(std::vector<Texture>::const_iterator it = _textures.begin();
		it != _textures.end();
		++it)
	{
		it->Bind();
	}
}

void ShaderMaterial::Unbind()
{
	if(_shaderProgram) _shaderProgram->Unbind();
}

ShaderMaterial::ShaderParameters& ShaderMaterial::GetVSParameters()
{
	return _vsParameters;
}

ShaderMaterial::ShaderParameters& ShaderMaterial::GetPSParameters()
{
	return _psParameters;
}