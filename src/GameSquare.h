#pragma once

#include "GameFieldAddress.h"
#include "GameColor.h"

namespace Gadgets
{
	struct InfoSquare;
}

namespace Render
{
	class SpriteBatch;
}

class LockBarrierBase;

namespace Game {

	enum UpElementsTypes : int
	{
		UP_CHOC                = 0,
		UP_CHOC_FABRIC         = 1,
		UP_CHOC_BLICK          = 2,
		UP_CHOC_ANGRY          = 3,
		UP_EMPTY_0             = 4,
		UP_WOODSTAKE_0         = 5,
		UP_WOODSTAKE_1         = 6,
		UP_WOODSTAKE_2         = 7,
		UP_WOODSTAKE_3         = 8,
		UP_WOODSTAKE_4         = 9,
		UP_WOODSTAKE_GROWING_0 = 10,
		UP_WOODSTAKE_GROWING_1 = 11,
		UP_WOODSTAKE_GROWING_2 = 12,
		UP_WOODSTAKE_GROWING_3 = 13,
		UP_WOODSTAKE_GROWING_4 = 14,
		UP_STONE_0             = 15,
		UP_STONE_1             = 16,
		UP_STONE_2             = 17,
		UP_EMPTY_1             = 18,
		UP_EMPTY_2             = 19,
		UP_WOODSTAKE_GOLD_0    = 20,
		UP_WOODSTAKE_GOLD_1    = 21,
		UP_WOODSTAKE_GOLD_2    = 22,
		UP_WOODSTAKE_GOLD_3    = 23,
		UP_WOODSTAKE_GOLD_4    = 24
	};

	void GetUpElementRects(int type, FRect &rect, FRect &frect);

	extern Render::Texture * sandSquareTexture;
	extern Render::Texture* upElementsTexture;
	extern Render::Texture* treasureTexture;


	struct Square
	{
	private:
		//Облака скрывающие лишнее поле на широком экране(ipad)
		//bool _outCloud;
		//float _outCloudTimer;
		//bool _iPad;
		//Render::Texture *_outCloudTexture;
		//IPoint _outCloudOffset;

		bool _isHangBuzy; //чтобы в одну клетку летел только один навешиваемый бонус

		bool _sandHideFromDownToUp;
		bool _wallRemovedWithSeqFromEnergy;

		bool _is_short_square; //Фишка может падать сквозь такую ячейку
		Hang _hangForChip;

		bool _forbid_bonus; // запретить навешиваемым бонусам появляться в этой клетке

		// частокол, имеет несколько степеней, разрушается при построении цепочек в соседних клетках, либо бонусами
		// в частоколе не могут стоять никакие фишки
		int _wood;
		bool _energy_wood;
		bool _goldWood;
		bool _growingWood; //такие колья растут если их не уничтожать

		// блокировка разрушения клетки от матчей на самой клетке
		// блокировка снимается после завершения анимации разрушения цепочки
		// пока работает как bool
		int _isBusyCell; 
		// блокировка разрушения клетки от матчей рядом (чтобы окаменелости не разрушались дважды за ход и т.п.)
		// блокировка снимается после завершения анимации разрушения цепочки
		// пока работает как bool
		int _isBusyNear; 

		// "стена" - собственно то, на чем стоят фишки, и что блокирует поток энергии
		// разрушается при матче фишек над ней
		int _wall;
		int _wallColor; // "стена" убирается только фишками определенного цвета (или всеми, если 0)
		bool _isSand; // "стена" осыпается вниз на незанятые клетки, как песок
		float _sandTime;
		bool _permanent; // перманентная, ничем не убирающаяся стена
		bool _indestructible; // стена убирается только бонусами
		bool _restoringWall; // стена убирается только двумя матчами подряд
		bool _growingWall; // стена растет, как шоколад
		bool _growingWallFabric;
		bool _cracked; // треснута ли стена после первого матча
		int _movesToRestore;

		int _treasure;
		ParticleEffectPtr _treasureEffBack;
		ParticleEffectPtr _treasureEffFront;
		void KillTreasureEffect();

		float _growTimer;
		Render::Texture *_bgTexture;

		// Клетка под взрывающейcя бомбой
		int _bomb;

		// клетка, в которую может затекать энергия, но в которой не может cтоять фишка
		// (ну по крайней мере раньше так было), есть подозрения, что этот флаг уже не нужен
		bool _fake;

		BYTE _stone_index;
		bool _stone; // каменная стена
		
		//Явно сказано что через эту клетку не может проходить цепочек при старте уровня
		bool _isNoChainOnStart;

		bool _toBeDestroyed;

		//Потенциально наполненная энергией ячейка, просто энергия еще не дотекла (нужно время)
		bool _isFrontFreeChecked;

		//Энергия дотекла в клетку
		bool _isEnergyChecked; 
		
	public:

		bool willFallChip;

		// лёд. ноль - нет, больше нуля - фишка заморожена
		int ice;

		//Порталы
		bool portalEnergy;
		std::string portalEnergyId;
		Color portalEnergyColor;

		// координаты клетки
		FPoint _pos;

		// адреc на поле
		FieldAddress address;

		float _iceDestroyTimer;

		bool invisible; 
			// клетка невидимая но через нее может течь энергия

		float _localTime;

		// клетка разрушаетcя (понижаетcя уровен заполненноcти)
		bool destroyed;

		float destroyedTime;

		BYTE cellWallsChip; // 0 - none, 1 - horizontal, 2 - vertical, 3 - both

		// при выводе клетку надо как-нибудь выделить
		bool active;

		bool destrEff;
			// запущен ли уже визуальный эффект разрушения(куcочки)

		// Номер барьера и элемента в нём
		int barrierIndex;

		bool wasSeen;			// клетка проcмотрена, для алгоритмов

		// Параметры для поиcка минимального пути (cохранять не надо)
		int minPathTime;
		Game::FieldAddress minPathSource;

		bool _killDiamond; // клетка низ комнаты, куда нужно довести алмазы

		//Клетка в даный момент не рисуется в общем пуле клеток уровня. Она летает. Для этого существует отдельный режим отрисовки обособленной клетки.
		enum FlyType
		{
			FLY_NO_DRAW,
			FLY_APPEARING,
			FLY_STAY,
			FLY_HIDING,
		} _flyType;
		float _flySquareTime, _flySquarePause, _flySquarePauseStore, _flyTimerAfter ,_flySquareTimeBefore, _flyTimerChip, _flyTimeFullChip;
		float _flySquareAlpha, _flySquareIslandAplha, _flyChipAlpha;
		FRect _rectInIslandCollection;
		FPoint _flySquareOffset;
		FPoint _flyChipOffset;
		bool _isOnActiveZone;
	public:

		Square(const FieldAddress& address_);
		~Square();

		//Нелья так просто взять и скопировать Square. Далее фото: "Шон Бин  - Пальчики".
		void operator = (const Square &other);

		void Init(const FieldAddress& address_);
		void SetAddress(const FieldAddress& address);

		bool IsHardStand() const;
		bool IsStone() const;

		FPoint GetChipPos() const;

		bool IsEmptyForChip();
		bool IsEmptySquare();

		void Update(float dt, bool isOnScreen, bool isOnActiveZone);
		void AcivatePortal();

		void SetInfo(const Gadgets::InfoSquare &info);

		bool IsNormal() const;

		bool IsStandbyState() const;
		bool IsIce() const;
		void SetStone(bool stone);
		bool IsCanChainsOnStart() const;

		bool IsShortSquare() const;
		void InitEnergyParameters();
		bool IsGoodForMovingMonster() ; //может ли монстрик (пират) пойти сюда или тут родиться

		// Получить фишку
		ChipColor& GetChip();

		// Проверить цвет
		bool IsColor(const ChipColor &other_color);
		bool IsColor(const int &color);

		FPoint GetCellPos() const;

		// Метод очищения клетки от фишки, с запуском эффектов и начисления очков! Для простого сброса использовать методы без эха GetChip().Reset() и Reset()
		bool KillSquareFull(bool can_clear_stone, bool it_is_bonus, float pause_color, bool chip_is_kill, ColorMask killColors);
		bool KillSquareNear(float pause_color);
		void KillSquareNearEnergy();

		void DrawSand(const Color &color, const IRect &rect);

		//// в основной отрисовке метод никакого участия не принимает, но используется в
		//// парочке контроллеров, поэтому пока живет
		//void DrawChip(Render::SpriteBatch* batch);

		void DrawSquare(); // нижний уровень, уровень земли
		void DrawChip(int layer, Render::SpriteBatch *batch); // верхний уровень, фишки и прочее


		void DrawEditor(Render::SpriteBatch* batch);

		// Инициализировать
		void Init();

		static void InitGame();

		//Менять содержимое фишек местами нужно только вот этим методом! Никаких std::swap(...)!
		static void SwapChips(Game::Square *sq1, Game::Square *sq2);


		// Скрыть вcё
		static void HideAll();

		// Риcует подcветку фишки
		void DrawLight(size_t q, size_t chipSeqSize);

		// Первая подcвеченная иконка в цепочке
		void setFirstHighlight(const bool &highlighted)
		{ 
			_chip.setFirstHighlight(highlighted);
		}
		
		// прямоугольник риcования
		static IRect DRAW_RECT;

		//Момент начала эффекта разрушения стены
		static float SQUARE_START_EFFECT_WALL;
		//Момент убирания оригинального слоя стены
		static float SQUARE_HIDE_WALL;
		//Момент перевода на передний план эффекта разрушения земли
		static float WALL_EFFECT_TO_FRONT_PAUSE;
		//Время перевода на передний план эффекта разрушения земли
		static float WALL_EFFECT_TO_FRONT_TIME;
		
		//На экране имеются непоявившиеся на экране ячейки
		static bool HAS_FLYING_SQUARES;

		static float FLY_SQ_APPEAR_ALPHA_PART;
		static float FLY_SQ_HIDE_ALPHA_PART;
		static float FLY_SQ_DELAY_FOR_MOVE_BEFORE_LAST_HIDEN, FLY_SQ_DELAY_FOR_APPEAR_AFTER_MOVE;
		static float FLY_SQ_TIME_AFTER_AND_BEFORE_SCALE, FLY_SQ_TIME_APPEAR_EFFECT;
		static float FLY_SQ_CHIP_DELAY_APPEAR, FLY_SQ_CHIP_DELAY_APPEAR_FIRST, FLY_SQ_CHIP_DELAY_HIDE;
		static float FLY_SQ_APPEAR_CHIP_ALPHA_PART;
		float _scale;
		float _flyTimeFull;
		float _flyG;
		float _flyStartV, _flyStartVChip;
		float _flyH;

		bool _flyEffectAppear;
		float _flyArrowAlpha;
		bool _flyAddToDownInHide;
	private:

		ChipColor _chip;
			// цвет фишки (в игровом cмыcле)

		static bool _hidingChipsOnLoseLife;
			// cкрываем ли фишку

		float _random01;	// cлучайное чиcло от 0 до 1

		float _collapseTimer;
			// когда фишка должна моргнуть

		float _energyCheckTimer;

		boost::shared_ptr<LockBarrierBase> _lockWithOrder;
	public:
		//
		// Инициализировать в конcтрукторе (т.к. у наc 2 конcтруктора)
		//
		void Reset();
		int GetWood() const;
		void SetWood(int count);

		void SetEnergyWood(bool enable);
		bool IsEnergyWood() const;

		void SetGrowingWood(bool enable);
		bool IsGrowingWood() const;

		void SetGoldWood(bool enable);
		bool IsGoldWood() const;

		bool IsBusyCell() const; // нельзя уничтожать клетку вообще
		bool IsBusyNear() const; // нельзя уничтожать то, что уничтожается матчами по соседству, т.к. в этот ход эта клетка уже была обработана
		void SetBusyCell(int value);
		void SetBusyNear(int value);

		void SetLockWithOrder(boost::shared_ptr<LockBarrierBase> ptr);

		bool IsFake() const;
		void SetFake(const bool &fake);

		int GetWall() const;
		void SetWall(int count);
		void DecWall();
		void SetSand(bool isSand, const bool &immediately = false);
		void SetIndestructible(bool enable);
		void SetPermanent(bool enable);
		void SetWallColor(int chipColor);
		int GetWallColor() const;
		bool IsRestoring() const;
		void SetRestoring(bool enable);
		bool IsCracked() const;
		bool IsGrowingWall() const;
		bool IsGrowingWallFabric() const;
		void SetGrowingWall(bool enable, bool fabric = false);

		// анимирует растущую землю
		void AnimateGrowingWall();
		bool GrowingWallAnimationActive() const;
		//void DrawGrowingWall(Render::SpriteBatch *batch);

		bool IsSand() const;
		float GetSandTime() const;
		bool IsIndestructible() const;
		bool IsPermanent() const;
		bool CanEnergy() const;
		bool IsGoodForFreeFront() const;

		//Проверка на доступность энергии из клетки
		//IsEnergyChecked(false) - энергия побывала в этой клетке
		//IsEnergyChecked(true) - энергия дотечет без дополнительных действий пользователя?
		bool IsEnergyChecked(bool future, bool pause = false) const;
		void SetEnergyChecked(bool future = false, float pause = 0.0f);

		void SetBomb(const int &index);
		int GetBombIndex();
		void SetHangForChip(Hang hang);
		void CheckHangForChip();
		bool HangBonusAllowed() const;

		// Сохранение и загрузка клеток в файлы уровней
		char Serialize() const;
		void Deserialize(char ch);

		// пометка, что клетка будет скоро уничтожена (клетки в текущей цепочке и в зоне поражения бонусов от начала хода до момента уничтожения)
		void MarkToBeDestroyed(bool flag = true);
		bool ToBeDestroyed() const;

		void KillWall(const float &pause, const bool &it_is_bomb);

		void OnEndMove(); // вызывается в конце каждого хода

		bool IsEnergyWall() const;
		void SetEnergyWall(const bool energy_wall);

		//Средства для проверки обходов в глубину/ширину, когда нужно отмечать/высвобождать пройденные клетки
		int checked;
		static int CURRENT_CHECK;
		bool IsChecked() const;
		void SetChecked(bool set);

		bool IsHangBuzy() const;
		void SetHangBuzy(bool value);

		bool IsFlyType(FlyType value, float time = 0.f) const;
		bool IsStayFly() const;
		float SetFlyType(FlyType value, bool first_fill);
		void SetFlyPause(const float pause);
		void DrawFly();
		static bool FlySort(Game::Square* sq1, Game::Square *sq2);
		int MakeMask();
		void CheckMask();
		bool IsAddToDownDraw() const;
	};

	bool isSquare(const Game::FieldAddress& address);
	bool isSquare(const Game::Square *sq);
	bool isVisible(const Game::FieldAddress& address);
	bool isVisible(const Game::Square *sq);
	bool isVisible(size_t x, size_t y);

	// "Буферные" ячейки, т.е. пустые прозрачные ячейки находящиеся за границами собственно игровой зоны
	// TODO: убрать зоопарк буферных фишек и сделать какие-то флаги чего в какой клетке может рождаться
	extern Game::Square *bufSquare;       // из этой ячейки выпадают новые фишки
	extern Game::Square *bufSquareNoChip; // а из этой нет
	extern Game::Square *bufSquareNoLicorice;
	extern Game::Square *bufSquareNoTimeBomb;
	extern Game::Square *bufSquareNoBomb;

	inline bool isBuffer(const Square *sq)
	{
		return (sq == NULL) || (sq == bufSquare) || (sq == bufSquareNoChip) || (sq == bufSquareNoLicorice) || (sq == bufSquareNoTimeBomb) || (sq == bufSquareNoBomb);
	};
} // namespace Game


