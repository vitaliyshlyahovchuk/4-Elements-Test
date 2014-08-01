#include "stdafx.h"
#include "GameFieldController.h"

#include "Game.h"
#include "GameField.h"

GameFieldController::GameFieldController(const std::string  &name_, float time_scale_, GameField *gamefield_)
	: IController(Game::MakeControllerName(name_))
	, gameField(gamefield_)
	, time_scale(time_scale_)
	, z(2)
{
	local_time = 0.f;
	MyAssert(GameField::VALID);
	gameField->_controllers.push_back(this);
}

GameFieldController::~GameFieldController()
{
	if(!GameField::VALID)
	{
		//MyAssert(false); //ToDo: может с этим лучше как то обойтись
		return; //Геймфилда уже нет в помине
	}
	for (GameFieldControllerList::iterator it = gameField->_controllers.begin(); it != gameField->_controllers.end(); ++it)
	{
		if ((*it) == this)
		{
			gameField->_controllers.erase(it);
			break;
		}
	}
}
