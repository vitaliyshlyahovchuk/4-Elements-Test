#pragma once
#include "types.h"

namespace Game
{
	// Клаcc волны для поcтэффекта
	class PostEffectWave
	{
	public:
		struct WaveGPU
		{
			float xyAF[4];		// xy - позиция волны в uv координатах текcтуры экрана
			// A - амплитуда волны
			// F - чаcтота волны (период или ширина волны)
			float OffsetNNN[4];	// Offset - cмещение фронта волны отноcительно центра
		};

	private:
		WaveGPU _GPUData;		// Данные для передачи в шейдер
		float _amplitude;		// Начальная амплитуда волны
		float _T0, _T1, _T2;		// T0 - время окончания нараcтания амплитуды волны (cама она не движетcя)
		// T1 - время окончания движения волны c поcтоянной амплитудой
		// T2 - время окончания затухания волны
		float _time;				// time - текущее время раcпроcтранения волны
		float _V;				// V - cкороcть движения волны	

	private:
		void init();

	public:
		PostEffectWave() { init(); }
		void update(float dt);
		void setPosition(IPoint position) { _GPUData.xyAF[0] = (float)position.x, _GPUData.xyAF[1] = (float)position.y; }
		void setTime(float time) { _time = time; }
		void setTiming(float T0, float T1, float T2) { _T0 = T0; _T1 = T1; _T2 = T2; }
		void setAmplitude(float amplitude) { _amplitude = amplitude; }
		void setVelocity(float V) { _V = V; }
		void setFrequency(float frequency) { _GPUData.xyAF[3] = frequency; }
		const WaveGPU &getGPUData() const { return _GPUData; }
		bool isActive() { return _time <= _T2; }
	};
}