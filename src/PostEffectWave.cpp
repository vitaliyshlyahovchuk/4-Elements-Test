#include "stdafx.h"
#include "PostEffectWave.h"

namespace Game
{
	void PostEffectWave::init()
	{
		_GPUData.xyAF[0] = 0.0f;
		_GPUData.xyAF[1] = 0.0f;
		_GPUData.xyAF[2] = 0.0f;
		_GPUData.xyAF[3] = 1000.0f;
		_GPUData.OffsetNNN[0] = 0.0f;
		_T0 = 0.0f;
		_T1 = 0.2f;
		_T2 = 0.8f;
		_amplitude = 0.03f;
		_time = 0.0f;
		_V = 300.0f;
	}

	void PostEffectWave::update(float dt)
	{
		_time += dt;
		// Пока линейная завиcимоcть
		if (_time < _T0)
			_GPUData.xyAF[2] = _time / _T0;
		else if (_time < _T1)
			_GPUData.xyAF[2] = 1.0f;
		else
		{
			float t = math::clamp((_T2 - _time) / (_T2 - _T1), 0.0f, 1.0);
			_GPUData.xyAF[2] = t * t;
		}
		_GPUData.xyAF[2] *= _amplitude;
		if (_time < _T0)
			_GPUData.OffsetNNN[0] = 0.0f;
		else
			_GPUData.OffsetNNN[0] = _V * (_time - _T0);
	}

}
