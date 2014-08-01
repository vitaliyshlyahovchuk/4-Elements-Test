#ifndef ONCE_FLYING_THING
#define ONCE_FLYING_THING

#include "GameFieldController.h"
#include "Utils/IPoint.h"

// �������� ��� ����� ��� ������
// (� �������� �����������)

typedef void (*FlyingThingDrawFunction)(); //������� ��������� 
typedef void (*FlyingThingFinishFunction)(void *); //������� ���������� ��� ��������� 

class FlyingThing
	: public GameFieldController
{
	FPoint _posFrom;  //������
	FPoint _posTo;    //����
	FPoint _velocity; //��� ������
	FPoint _pos;      //��� ������

	enum { FLY, FINISHED } _state; //���������

	FlyingThingDrawFunction _drawer; //����������
	float _timeToFly; //�� ������� ������ ��������

	FlyingThingFinishFunction _finisher; //���������� ����� ��������
	void *_finishParams; //��������� _finisher
public:
	FlyingThing(FPoint from, FPoint to, float timeToFly, FlyingThingDrawFunction drawer, FlyingThingFinishFunction finisher, void *finishParams);

	void Init();
	void Update(float dt);
//	void DrawUnderChips();
	void DrawAbsolute();
	bool isFinish();
//	void AddMessageOnFinish(const Message &message);
};

#endif
