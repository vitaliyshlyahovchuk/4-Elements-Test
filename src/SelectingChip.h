#ifndef SELECTING_CHIP_H
#define SELECTING_CHIP_H

#include "GameFieldAddress.h"
#include "GameFieldController.h"

class SelectingChips
{
	float _timerNewLight;
	bool _needNewLight;
public:
	static void Init();
	void InitConstruct();
	void Draw(AddressVector &seq, float time);
	void Update(float dt);
	void Clear();
};

namespace Game
{
	struct Square;
}

class ChipInSequence 
	: public GameFieldController
{
private:
	FPoint _posFrom, _posTo;
	bool _draw_link;
	Game::Square *_sq;
	float _timerHide;
	float _localTime;
	Render::Texture *_backTex, *_linkTex;
	float _timerNewLight;
	EffectsContainer _effCont;
	ParticleEffectPtr _effInSelection;
public:
	ChipInSequence(IPoint index, bool draw_link , IPoint index_from);
	~ChipInSequence();
	void Update(float dt);
	void DrawUnderChips();
	bool isFinish();
	void Hide();
	typedef boost::shared_ptr<ChipInSequence> HardPtr;
};


#endif // SELECTING_CHIP_H