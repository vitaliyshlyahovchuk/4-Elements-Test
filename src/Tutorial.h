#ifndef __MATCH3_TUTORIAL_H__
#define __MATCH3_TUTORIAL_H__

#include "TutorialEffects.h"

namespace Tutorial
{

class LuaTutorial
{
	std::string _scriptName;

	enum WaitCondition {
		NONE,
		MOVE,
		TAP,
		RECEIVER,
		ENERGY,
		GAME_START,
		END,
		BOOSTSELECT,
		CHIPSELECT,
		RUNBOOST,
		FREESEQ
	};
	WaitCondition _wait;

	ShowDirectionController *dirArrow;
	ShowMoveFinger *finger;
	AreaHighlighter *areaHighlight;
	PointArrowController *pointArrow;
	ShowText *tapText;

	LuaThread *_thread;
public:
	LuaTutorial();
	~LuaTutorial();

	void Init(const std::string& scriptName);
	void Run();
	void End();

	std::string GetScriptName() const { return _scriptName; }

	void AcceptMessage(const Message& message);

	// Lua interface
	void SetThread(LuaThread *thread);

	void ShowDirection(FPoint from, FPoint to);
	void HideDirection();
	
	void ShowMove(luabind::object const& points, float startTime); // points - lua-массив IPoint
	void HideMove();

	void ShowArrow(luabind::object const& cells, bool screenPoints = false);
	void HideArrow();

	void ShowHighlight(luabind::object const &points, luabind::object const &rects, float opacity);
	void HideHighlight();

	void PauseChipFall();
	void UnpauseChipFall();

	void ShowTapText();
	void HideTapText();

	void BlockField();
	void UnblockField();

	void SetEnergySpeed(float speed);
	void SetHangBonusCells(luabind::object const& points);
	void SetHangBonusTypes(luabind::object const& types);
	void StartReceivers();

	void WaitForGameStart();
	void WaitForMove();
	void WaitForTap();
	void WaitForReceiver();
	void WaitForEnergy();
	void WaitForBoostSelect();
	void WaitForChipSelect();
	void WaitForRunBoost();
	void WaitForFreeSeq();
	WaitCondition GetWait();
	bool IsPaused() const;

	void SetTutorialHighlightChain(luabind::object const& points, bool chain);
};

extern LuaTutorial luaTutorial;

} // end of namespace
#endif