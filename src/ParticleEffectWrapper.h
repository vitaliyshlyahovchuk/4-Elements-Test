#pragma once

#include "ParticleSystem.h"

//
// ���cc-������ ��� ����������� ��������
//
class ParticleEffectWrapper
{

public:
	
	//
	// ���c�������
	//
	ParticleEffectWrapper(std::string  effectName, FPoint pos);

	//
	// ���c������� �� ���������
	// ������, c�������� ����� �������, c������c� ���������� � ��������� ������, ����
	// GetPosition() � Restart() ����� �cc������c�
	//
	ParticleEffectWrapper();

	//
	// ��������
	//
	void Update(float dt);

	//
	// ����c�����
	//
	void Draw();

	//
	// �������� �� ������
	//
	bool IsEnd();

	//
	// �c�������� �������
	//
	void SetPosition(FPoint pos);

	//
	// ������� ������� �������
	//
	FPoint GetPosition();

	//
	// ��������� ������
	//
	void Finish();

	//
	// ���������� ����� ������
	//
	void Kill();

	//
	// ����c���� ������ ������
	//
	void Restart();

	//
	// ����c���� ������ ������ � ����� �������
	//
	void RestartAtPos(FPoint pos);

	typedef boost::shared_ptr<ParticleEffectWrapper> SharedPtr;

private:

	typedef boost::shared_ptr<ParticleEffect> EffectPtr;

	EffectPtr _particleEffect;
		// c�� ������; c������ ��c���� ����������, ����� ��������� ����������
		// c����� c����c����� �������� �������� ����� ���������� (�������� ����� ����������)

	FPoint _startPos;
		// ��������� �������
};