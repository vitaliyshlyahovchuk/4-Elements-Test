#ifndef ONCE_FLYING_THING
#define ONCE_FLYING_THING

#include "GameFieldController.h"
#include "Utils/IPoint.h"

// летающее над полем что нибудь
// (в экранных координатах)

typedef void (*FlyingThingDrawFunction)(); //функция рисования 
typedef void (*FlyingThingFinishFunction)(void *); //функция вызываемая при окончании 

class FlyingThing
	: public GameFieldController
{
	FPoint _posFrom;  //откуда
	FPoint _posTo;    //куда
	FPoint _velocity; //как быстро
	FPoint _pos;      //где сейчас

	enum { FLY, FINISHED } _state; //состояние

	FlyingThingDrawFunction _drawer; //рисователь
	float _timeToFly; //за сколько должно долететь

	FlyingThingFinishFunction _finisher; //вызывается когда прилетит
	void *_finishParams; //параметры _finisher
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
