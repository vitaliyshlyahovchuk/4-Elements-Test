#include "stdafx.h"
#include "Tutorial.h"
#include "Game.h"
#include "Match3.h"
#include "GameField.h"
#include "GameInfo.h"
#include "EnergyReceivers.h"

namespace Tutorial
{

template<class T>
void LuaArrayToVector(luabind::object const& luaVector, std::vector<T> &vec)
{
	int type = luabind::type(luaVector);
	if( type == LUA_TTABLE )
	{
		// lua-массив элементов
		luabind::iterator itr(luaVector), end;
		for( ; itr != end; ++itr )
		{
			vec.push_back( luabind::object_cast<T>(*itr) );
		}
	}
	else if( type == LUA_TUSERDATA )
	{
		// просто одиночный элемент
		vec.push_back( luabind::object_cast<T>(luaVector) );
	}
}

LuaTutorial::LuaTutorial()
	: dirArrow(nullptr)
	, finger(nullptr)
	, areaHighlight(nullptr)
	, pointArrow(nullptr)
	, tapText(nullptr)
	, _thread(nullptr)
	, _wait(NONE)
{
}

LuaTutorial::~LuaTutorial()
{
}

void LuaTutorial::Init(const std::string& scriptName)
{
	_scriptName = scriptName;
}

void LuaTutorial::Run()
{
	_wait = NONE;
	if( !_scriptName.empty() ) {
		Core::LuaDoFile("scripts/tutorials/" + _scriptName);
	}
}

void LuaTutorial::End()
{
	// сами контроллеры прибьются в KillControllers
	dirArrow = nullptr;
	finger = nullptr;
	areaHighlight = nullptr;
	tapText = nullptr;
	pointArrow = nullptr;

	if(_thread && Core::luaThreadManager.FindThread(_thread))
		_thread->Terminate();
	_thread = nullptr;

	Core::LuaDoString("if gameTutorialBubble ~= nil then gameTutorialBubble:show(false) end");

	_wait = END;

	GameField::Get()->SetEnergyTimeScale(1.0f);
}

void LuaTutorial::SetThread(LuaThread *thread)
{
	_thread = thread;
}

void LuaTutorial::ShowDirection(FPoint from, FPoint to)
{
	HideDirection();

	dirArrow = new ShowDirectionController(from, to);
	dirArrow->Start();
	Game::AddController(dirArrow);
}

void LuaTutorial::HideDirection()
{
	if( dirArrow ) {
		dirArrow->End();
		dirArrow = nullptr;
	}
}

void LuaTutorial::ShowArrow(luabind::object const& cells, bool screenPoints)
{
	if(_wait == END)
		return;

	HideArrow();

	std::vector<IPoint> ivec;
	LuaArrayToVector<IPoint>(cells, ivec);

	pointArrow = new PointArrowController(ivec, screenPoints);
	Game::AddController(pointArrow);
}

void LuaTutorial::HideArrow()
{
	if( pointArrow ) {
		pointArrow->Finish();
		pointArrow = nullptr;
	}
}

void LuaTutorial::ShowMove(luabind::object const& points, float startTime)
{
	if(_wait == END)
		return;

	HideMove();

	std::vector<IPoint> ivec;
	LuaArrayToVector<IPoint>(points, ivec);

	// не даем игроку сделать никакой другой ход, кроме выделенного
	Game::ChipColor::tutorialChainLength = ivec.size();
	for(size_t i = 0; i < ivec.size(); ++i)
	{
		Game::FieldAddress fa(ivec[i]);
		Game::Square *sq = GameSettings::gamefield[fa];
		sq->GetChip().HighlightChain();
	}

	// запускаем анимацию пальца, показывающего ход
	std::vector<FPoint> fchain;
	for(size_t i = 0; i < ivec.size(); ++i)
	{
		fchain.push_back( GameSettings::ToScreenPos(FPoint(ivec[i]) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF) );
	}
	finger = new ShowMoveFinger(fchain, startTime);
	Game::AddController(finger);
}

void LuaTutorial::HideMove()
{
	Game::ChipColor::tutorialChainLength = 0;
	if( finger ) {
		finger->Finish();
		finger = nullptr;
	}
}

void LuaTutorial::SetTutorialHighlightChain(luabind::object const& points, bool chain)
{
	if(_wait == END)
		return;

	Game::ChipColor::tutorialChainLength = 0;

	std::vector<IPoint> ivec;
	LuaArrayToVector<IPoint>(points, ivec);

	if (chain) Game::ChipColor::tutorialChainLength = ivec.size();

	for(size_t i = 0; i < ivec.size(); ++i)
	{
		Game::FieldAddress fa(ivec[i]);
		Game::Square *sq = GameSettings::gamefield[fa];
		sq->GetChip().HighlightChain();
	}
}

void LuaTutorial::ShowHighlight(luabind::object const& points, luabind::object const& luaRects, float opacity)
{
	if(_wait == END)
		return;

	std::vector<IPoint> ivec;
	LuaArrayToVector<IPoint>(points, ivec);

	std::vector<IRect> rects;
	for(size_t i = 0; i < ivec.size(); ++i)
	{
		IPoint pt = GameSettings::ToScreenPos(ivec[i] * GameSettings::SQUARE_SIDE);
		IRect rect(pt.x, pt.y, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);
		rect.Inflate(10);
		rects.push_back(rect);
	}

	std::vector<IRect> irects;
	LuaArrayToVector<IRect>(luaRects, irects);
	rects.insert(rects.end(), irects.begin(), irects.end());

	if(! areaHighlight ) {
		areaHighlight = new AreaHighlighter(rects, opacity);
		Game::AddController(areaHighlight);
	} else {
		areaHighlight->SetShape(rects);
	}
}

void LuaTutorial::HideHighlight()
{
	if( areaHighlight ) {
		areaHighlight->Hide();
		areaHighlight = nullptr;
	}
}

void LuaTutorial::PauseChipFall()
{
	Match3::Pause();
}

void LuaTutorial::UnpauseChipFall()
{
	Match3::Unpause();
}

void LuaTutorial::ShowTapText()
{
	if(_wait == END)
		return;

	HideTapText();

	std::string device_id = gameInfo.getGlobalString("device_id");
	int textY = 130;
	if( device_id == "iphone5" ) {
		textY = 210;
	} else if( device_id == "ipad" ) {
		textY = 150;
	}
	tapText = new ShowText(FPoint(Render::device.Width() / 2, textY), "TutorialTapToContinue");
	Game::AddController(tapText);
}

void LuaTutorial::HideTapText()
{
	if( tapText ) {
		tapText->Finish();
		tapText = NULL;
	}
}

void LuaTutorial::BlockField()
{
	GameField::Get()->BlockField();
}

void LuaTutorial::UnblockField()
{
	GameField::Get()->UnblockField();
}

void LuaTutorial::SetEnergySpeed(float speed)
{
	GameField::Get()->SetEnergyTimeScale(speed);
}

void LuaTutorial::SetHangBonusCells(luabind::object const& points)
{
	std::vector<IPoint> ivec;
	LuaArrayToVector<IPoint>(points, ivec);

	GameField::Get()->_tutorialHangBonusSquares.clear();
	for(IPoint pt : ivec)
	{
		GameField::Get()->_tutorialHangBonusSquares.push_back( Game::FieldAddress(pt) );
	}
}

void LuaTutorial::SetHangBonusTypes(luabind::object const& types)
{
	std::vector<std::string> svec;
	LuaArrayToVector<std::string>(types, svec);

	GameField::Get()->_tutorialHangBonusTypes.clear();
	for(std::string& type : svec)
	{
		GameField::Get()->_tutorialHangBonusTypes.push_back( type );
	}
}

void LuaTutorial::StartReceivers()
{
	Gadgets::receivers.AcceptMessage( Message("Start", "from_tutorial") );
}

void LuaTutorial::WaitForGameStart()
{
	if(_wait != END)
		_wait = GAME_START;
}

void LuaTutorial::WaitForMove()
{
	if(_wait != END)
		_wait = MOVE;
}

void LuaTutorial::WaitForTap()
{
	if(_wait != END)
		_wait = TAP;
}

void LuaTutorial::WaitForReceiver()
{
	if(_wait != END)
		_wait = RECEIVER;
}

void LuaTutorial::WaitForEnergy()
{
	if(_wait != END)
		_wait = ENERGY;
}

void LuaTutorial::WaitForBoostSelect()
{
	if(_wait != END)
		_wait = BOOSTSELECT;
}

void LuaTutorial::WaitForChipSelect()
{
	if(_wait != END)
		_wait = CHIPSELECT;
}

void LuaTutorial::WaitForRunBoost()
{
	if(_wait != END)
		_wait = RUNBOOST;
}

void LuaTutorial::WaitForFreeSeq()
{
	if(_wait != END)
		_wait = FREESEQ;
}

LuaTutorial::WaitCondition LuaTutorial::GetWait()
{
	return _wait;
}

bool LuaTutorial::IsPaused() const
{
	return (_wait != NONE) && (_wait != END);
}

void LuaTutorial::AcceptMessage(const Message &message)
{
	if( message.is("OnTap") ) {
		if(_wait == TAP ) {
			_wait = NONE;
		}
	} else if( message.is("OnStartMove") ) {
		if(_wait == MOVE ) {
			_wait = NONE;
		}
	} else if( message.is("OnReceiverActivated") ) {
		if(_wait == RECEIVER ) {
			_wait = NONE;
		}
	} else if( message.is("OnEnergyStop") ) {
		if(_wait == ENERGY ) {
			_wait = NONE;
		}
	} else if( message.is("OnGameStart") ) {
		if(_wait == GAME_START ) {
			_wait = NONE;
		}
	} else if( message.is("OnBoostSelect") ) {
		if(_wait == BOOSTSELECT ) {
			_wait = NONE;
		}
	} else if( message.is("OnChipSelect") ) {
		if(_wait == CHIPSELECT ) {
			_wait = NONE;
		}
	} else if( message.is("OnRunBoost") ) {
		if(_wait == RUNBOOST ) {
			_wait = NONE;
		}
	} else if( message.is("OnFreeSeq") ) {
		if(_wait == FREESEQ ) {
			_wait = NONE;
		}
	}
}

LuaTutorial luaTutorial;

} // end of namespace