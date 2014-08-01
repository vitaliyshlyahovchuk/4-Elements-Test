#include "stdafx.h"
#include "ParticleEffectWrapper.h"

ParticleEffectWrapper::ParticleEffectWrapper(std::string  effectName, FPoint pos)
	: _particleEffect(new ParticleEffect(effectPresets.getParticleEffect(effectName)))
	, _startPos(pos)
{
	SetPosition(pos);
	_particleEffect->Reset();
//	_particleEffect->Update(0.0f);
}

ParticleEffectWrapper::ParticleEffectWrapper()
	: _particleEffect()
{
}

void ParticleEffectWrapper::Update(float dt) {
	if (_particleEffect != NULL && !_particleEffect->isEnd()) {
		_particleEffect->Update(dt);
	}
}

void ParticleEffectWrapper::Draw() {
	if (_particleEffect.get() != NULL && !_particleEffect->isEnd()) {
		_particleEffect->Draw();
	}
}

bool ParticleEffectWrapper::IsEnd() {
	return _particleEffect.get() == NULL || _particleEffect->isEnd();
}

void ParticleEffectWrapper::SetPosition(FPoint pos) {
	Assert(_particleEffect.get() != NULL);
	if (!_particleEffect->isEnd()) {
		pos.GetXY(_particleEffect->posX, _particleEffect->posY);
	}
}

FPoint ParticleEffectWrapper::GetPosition() {
	Assert(_particleEffect.get() != NULL);
	return FPoint(_particleEffect->posX, _particleEffect->posY);
}

void ParticleEffectWrapper::Finish() {
	if (_particleEffect.get() != NULL) {
		_particleEffect->Finish();
	}
}

void ParticleEffectWrapper::Kill() {
	if (_particleEffect.get() != NULL) {
		_particleEffect->Kill();
	}
}

void ParticleEffectWrapper::Restart() {
	Assert(_particleEffect != NULL);
	SetPosition(_startPos);
	_particleEffect->Reset();
	_particleEffect->Update(0.0f);
}

void ParticleEffectWrapper::RestartAtPos(FPoint pos) {
	Assert(_particleEffect != NULL);
	pos.GetXY(_particleEffect->posX, _particleEffect->posY);
	_particleEffect->Reset();
	_particleEffect->Update(0.0f);
}