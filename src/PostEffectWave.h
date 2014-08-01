#pragma once
#include "types.h"

namespace Game
{
	// ���cc ����� ��� ��c��������
	class PostEffectWave
	{
	public:
		struct WaveGPU
		{
			float xyAF[4];		// xy - ������� ����� � uv ����������� ���c���� ������
			// A - ��������� �����
			// F - ��c���� ����� (������ ��� ������ �����)
			float OffsetNNN[4];	// Offset - c������� ������ ����� ����c������� ������
		};

	private:
		WaveGPU _GPUData;		// ������ ��� �������� � ������
		float _amplitude;		// ��������� ��������� �����
		float _T0, _T1, _T2;		// T0 - ����� ��������� ����c����� ��������� ����� (c��� ��� �� ������c�)
		// T1 - ����� ��������� �������� ����� c ��c������� ����������
		// T2 - ����� ��������� ��������� �����
		float _time;				// time - ������� ����� ��c���c�������� �����
		float _V;				// V - c����c�� �������� �����	

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