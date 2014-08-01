#pragma once
#include "GameFieldAddress.h"
#include "GameBonus.h"
#include "MovingMonster.h"
#include "ChipDistortions.h"

class ChipInSequence;

namespace Gadgets
{
	class ChipsInfo;
	struct InfoC;
}

namespace Render
{
	class SpriteBatch;
}

namespace Game
{
	enum ChipColors : int // отражает на каком месте в текстуре Chips.png находятся какие виды фишек
	{
		KEY      = 55,
		MUSOR    = 56,
		TREASURE = 57,
		MONSTER  = 58,
		LICORICE = 59,
		SNOW     = 60,
		DIAMOND  = 61,
		ADAPT    = 62,
	};
	
	//Все что принадлежит фишке находится в ChipColor.
	//Все содержимое падает вместе с фишкой.
	class ChipColor
	{
	public:
		enum Type : int
		{
			CHIP      = 0,
			LICORICE  = 1,
			KEY       = 2,
			TREASURE  = 4,
			THIEF     = 5,
			STAR      = 6,
			MUSOR     = 7,
			DIAMOND   = 8
		};
	private:
		bool _isOnActiveZone;

		std::list<boost::intrusive_ptr<ChipDistortion>> _distortions;

		Thief::HardPtr _thief;

		float _timerForHideWhiteBack;
		Hang _hangForSpirit;
		Hang _hang; // Навешиваемый бонус.
		Hang *_hang_current;
		BYTE _hang_current_dir;
		
		Type _type;

		bool _light; // светлячок на фишке
		int _act_count;
		int _value;
		bool _isAdapt; 
		bool _isChameleon;
		int _musor_index;
		bool _mirror;
		int _starAct;
		int _treasureLevel;
		float _starActLightTimer;

		int _lock;
		int _time_bomb;

		bool _preinstalled;

		FPoint _lastChipPos;
		float _localTime;
		FPoint _offset;
		FPoint _scale;
		bool _blicked; // на фишке еcть блик
		float _blickTime;
		// подcвеченноcть (фишка анимируетcя)
		bool _firstHighlighted;

		//анимация выделения
		bool _playSelected;
		bool _playSelectedForward; //Пока анимация полностью не проиграет, обратная анимация не начнется
		float _playSelectedTime;

		float _fallingPause; //cколько надо подождать перед началом падения
		enum FallState
		{
			FALL_NO,
			FALL_DOWN,
			FALL_AFTER,
			FALL_MOVE, //двигается (например, обмен с пиратом)
		}_fallingState;
		FPoint _acc, _velocity;
		FPoint _chipPos;
		FPoint _movingFromPos; //откуда начал двигаться (чтобы засечь когда закончит движение)

		float _timeChipFalling;

		bool _fly; // фишка в полете, т.е. ее позиция контролируется извне (решафл, бонус поменять фишки местами)

		int _inSequence;

		bool _tutorialHighlight;

		Render::StreamingAnimationPtr _chipAnim;

		std::vector<FPoint> _flyPoints;

	public:
		int _selected;

		static float YOffset_Chip, YOffset_ChipBack, YOffset_ChipBackIce;
		static float YOffset_Choc, YOffset_ChocFabric;
		static float YOffset_WOOD[5];
		static float YOffset_STONE;
		static float CHIP_REMOVE_DELAY;
		static IPoint CHIP_TAP_OFFSET;

		static float CHIP_START_HIDE;


		//CURRENT_IN_SEQUENCE_ITER работает в связке с _inSequence. Проверяется состояние их равенства. Для того чтобы обнулить условие достаточно проинкрементировать CURRENT_IN_SEQUENCE_ITER
		static int CURRENT_IN_SEQUENCE_ITER;
		static bool chipSeqIsEmpty;

		static size_t tutorialChainLength; // длина показанной в туториале цепочки (0 - нет цепочки)

		static Render::Texture* chipsTex;

		static int COLOR_FOR_ADAPT;
		float _chipAlpha, _fChipAlpha, _chipAlphaTo;
		bool checkFall;

		static IRect DRAW_RECT;
		static FRect DRAW_FRECT;
		static void InitGame(rapidxml::xml_node<> *xml_node);
		static float timeAfterLastFallSound;

		void DrawWhiteBorder(FPoint pos, bool in_ice);

		EffectsContainer _innerEffCont;
		//ParticleEffectPtr _effInSelection;
		ChipInSequence* _chipInSequence;

	private:
		// эффект, которым выделяется бонусная фишка в выделенной цепочке
		ParticleEffectPtr _seqBonusEff, _seqBonusEffDown;
		//Рисуем нужную ячейку с текстуры
		void DrawHangUnder(int value, const FPoint &pos, Render::SpriteBatch* batch, bool active, bool in_ice) const;
		void DrawChipFixedColor(int value, const FPoint &pos, Render::SpriteBatch* batch, bool active = false, bool in_ice = false, bool has_hang = false) const;
		void DrawLightEffect(int value, FRect draw_rect, FRect uv) const;
		void DrawDiagonalBlic(int value, FPoint pos, int countX, int countY, float fi, float time, int alpha, Color c, Render::SpriteBatch* batch, bool eye_open, bool in_ice);
		
		// легкий отскок фишки в момент приземления
		//float _bounceTimer;
		//float _bounceAmp;

		void UpdateDrawHang(Game::FieldAddress address);
	public:

		ChipColor(int value);
		void Reset(bool reset_hard);
		void ResetColor(); // сбрасывает только параметры фишки, отвечающие за ее тип/цвет
		void SetMusor(bool musor);
		bool IsMusor() const;
		bool IsKey() const;
		bool IsSimpleChip() const;
		bool IsExist() const ;
		bool IsAdapt() const;
		void SetAdapt(bool adapt);
		bool IsChameleon() const ;
		void SetChameleon(bool isChameleon);
		int GetColor() const;
		bool IsColor(const int &value) const;
		void Draw(const FPoint &pos, const Game::FieldAddress &address, Render::SpriteBatch* batch, bool in_ice);
		void DrawOver();
		void BreakStarArrow();

		// Первая подcвеченная иконка в цепочке
		void setFirstHighlight(const bool &highlighted);
		void Update(float dt, const Game::FieldAddress &address, bool isOnScreen, bool isOnActiveZone);

		void Select(int seqNumber, IPoint index, bool prev_exist = false, IPoint index_prev = IPoint(-1, -1));
		void Deselect();
		//Проверить и если нужно удалить эффект бонуса в цепочке
		void CheckKillSeqEff();
		bool IsFutureHang() const;

		// Проиграть анимацию включения в цепочку
		void PlaySelected(bool selected);

		// Подсветка цепочки фишек в туториале
		void HighlightChain();
		bool IsChainHighlighted() const;

		// позиция относительно родительской клетки, используется для падения, полетов и т.п.
		void SetPos(FPoint pos);
		void MoveBy(FPoint offset);
		FPoint GetPos() const;

		// запускает падение из текущей позиции (заданной в MoveBy или SetPos) к нулевой позиции
		void RunFall(float pause, float velocity, float acceleration);
		void AddFlyWaypoint(FPoint pos);

		// запускает передвижение из текущей позиции (заданной в MoveBy или SetPos) к нулевой позиции
		void RunMove(float pause, float velocity, float acceleration);

		// пазуа, заданная в RunFall
		float GetFallingPause() const;
		
		void CheckBlick();
		
		bool IsFly() const;
		bool IsFalling() const;
		bool IsStand() const;

		// вкл/выкл состояние полета, когда позиция фишки анимируется извне (решафл, замена фишек местами)
		void SetFly();
		void ResetFly();
		
		void SetAlpha(float fChipAlpha, float chipAlpha, float chipAlphaTo);
		void SetColor(int color);
		void SetInfo(const Gadgets::InfoC &info, IPoint index);

		// сдвиг отвечающий за дрожание клеток в цепочке, радиусе поражения бомбы и т.п.
		void SetOffset(float x, float y);
		void SetScale(float scale_x, float scale_y);
		void ClearOffset();
		FRect GetDrawRect(int type, bool in_ice, bool hang_pulse = false) const;

		//Так нужно убивать клетку - назначать ее исчезновение, разлетание эффекты...
		bool KillChip(const FPoint &offset_exploid, const Game::FieldAddress &address_parent_cell, bool it_is_bomb, bool near_match, float pause);
		void RunBonus(const Game::FieldAddress &address_parent_cell);

		Type GetType() const { return _type; }

		bool IsStarAct() const;
		int GetStarAct() const;
		bool IsLock() const;
		int GetLock() const;
		void SetLicorice();
		bool IsLicorice() const;

		int GetTreasure() const;

		void SetDiamond();

		Thief::HardPtr GetThief();
		bool IsThief() const;
		void SetThief(IPoint index);

		void SetLight();
		bool IsLight() const;
		void SetTimeBomb(int moves);
		bool IsTimeBomb() const;
		//есть ли навешенная бомба
		bool HasHangBomb() const; 

		void SetHang(const Hang &hang);
		bool HasHang() const;
		Hang& GetHang();

		bool HasDrawHang(const Game::FieldAddress &address);

		void OnEndMove(bool onScreen);
		void UpdateInSequence();
		bool IsProcessedInSequence();

		bool IsPreinstalled() const;

		bool IsEnergyBonus() const;

		void SetSpirit(Game::Hang hang);
		
		void AddDistortion(boost::intrusive_ptr<ChipDistortion> dist);

		float GetDistortionsAlpha(bool &show_eye_for_anim);
	};
} // namespace Game
