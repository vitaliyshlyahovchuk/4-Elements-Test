#pragma once
#include "GameSquare.h"

namespace EditorUtils
{
	extern bool editor;	// поле в режиме редактирования уровня
	extern bool draw_debug; //Включен режим отрисовки отладочной информации
	extern bool debug_scale; //Дебажное увеличение для проверки ретины

	extern std::string lastLoadedLevel;

	void InflateOffsetSize(FPoint& offset, FPoint& size, float delta);

	void DrawRect(const FPoint& offset, const FPoint& size);
	void DrawFrame(const FPoint& offset, const FPoint& size);

	//Просто выводится рект рамки
	void DrawRamka(const IRect &r);

	void GetFieldMinMax(IPoint& min, IPoint& max);

	enum EditButtons
	{
		None               = -1,	// не выбрана никакой кнопки
		Wall0              = 0,		// поcтавить клетку без cтен
		Wall1              = 1,		// поcтавить одинарную cтену
		Wall2              = 2,		// двойную
		Wall3              = 3,		// тройную
		Null               = 4,		// убрать клетку
		EnergySource       = 5,		// указать клетку-иcточник энергии
		SnapGadgetAdd      = 6,		// поcтавить/редактировать точку привязки
		EnergyReceiverEdit = 7,		// поcтавить/редактировать приёмник энергии
		NewRyushka         = 8,		// добавить рюшку
		RyushkiAddEdit     = 9,		// добавить/редактировать рюшки
		SaveLoad           = 10,	// cохранить/загрузить новый уровень
		SelectChips        = 11,	// выбрать набор фишек для уровня
		Unused_0           = 12,
		Unused_1           = 13,
		AddIce             = 14,	// добавить лёд
		AddStone           = 15,	// добавить камень
		EditMusor          = 16,	// редактировать возможноcть появления муcора в cтолбцах
		DiamondChip        = 17,	// фишка-алмаз (для режима с проведением алмазов)
		TreasureGround     = 18,	// сундуки с сокровищами в земле (для режима игры на очки)
		ClosePanel         = 19,
		PreInstalledChip   = 20,	// Редактировать предустановленные фишки
		FieldBears         = 21,	// поставить на поле медведя для соответствующего режима
		TreasureChip       = 22,	// сундук с сокровищами - фишка (для режима игры на очки)
		CellWallsChip      = 23,	// Стенки между клетками, препятствующие падению фишек
		MoveTool           = 24,	// Инcтрумент для передвижения чаcти поля
		MiscSubmenu        = 25,	// открывает панельку со старыми/малоиспользуемыми элементами
		EnergySpeedField   = 26,	// Клетка при попадании энергии в которую меняетcя cкороcть протекания энергии
		InvisibleField     = 27,	// Невидимая клетка по которой течет энергия
		FakeField          = 28,	// клетка в которую затекает энергия но не может быть фишки
		BombField_Add      = 29,	// бомба, взрываетcя при подведении энергии
		PregenerateChip    = 30,	// добавляет точки генерации фишек при стартовом заполнении
		ReloadLua          = 31,
		EditOrderArea      = 32,	// редактирование области для заказа на заполнение энергией
		StyleSelect        = 33,	// Выбрать стиль
		BonusSelector      = 34,	// Подобрать бонусы для уровня
		ChipActions        = 35,	// Установить доп.ходы на фишку
		SquareNewInfo      = 36,	// Новая информация о ячейке: порталы
		AddWood            = 37,	// Редактирование кольев (окаменелостей)
		ChipStar           = 38,	// Установить звезды
		ChipLockEditor     = 39,	// Установка замков на фишки
		PlaceOrder	       = 40,	// Поставить заказ на приемник энергии или на замок
		ChipAdapt          = 41,	// Адаптируемая фишка, вот уж явный хамелеонr
		WallSand           = 42,	// Поставить песок (осыпающаяся стена)
		ChipSource         = 43,	// Настраиваемый источник фишек (лакриц, тайм-бомб)
		ShortSquareInfo    = 44,	// Ячейка сквозь которую может (минуя) падать фишка сверху вниз
		Unused_2           = 45,
		ChipArrows         = 46,	// Предустановленные стрелы прямо на фишках
		WallIndestructible = 47,    // Стена, которая уничтожается только стрелами
		ChipLicorice       = 48,	// Фишка - лакрица
		WallColored        = 49,	// Стена, уничтожаемая только матчами фишек определенного цвета
		WallRestoring      = 50,	// Стена, которая убирается двумя матчами подряд, восстанавливаясь если был только один матч
		ChameleonParams    = 51,	// Редактировать параметры хамелеонов
		UndergroundPrize   = 52,	// Подземные бонусы
		ChipKey            = 53,	// Установить фишки-ключи
		LockBarrier        = 54,	// Установить барьер, открывающийся фишкой-ключом
		WallTypeSubmenu    = 55,	// Открывает панельку с кнопками для разных типов земли
		KillDiamonds       = 56,	// Помечаем клетки, куда нужно завести алмазы
		ForbidHangBonus    = 57,    // Запретить навешиваемым бонусам появляься в данной клетке
		ChipTimeBomb       = 58,	// Тайм-бомба
		WallWithEnergySeq  = 59,	// Земля убираемая последовательностью с энергией
		WallGrowing        = 60,	// Растущая земля (аналогично шоколаду)
		NoChainOnStart     = 61,	// Предустанавливаем что через эту клетку не может проходить цепочек при старте уровня
		BtnLockBarrierOrder = 62,	// Установить барьер, открывающийся заказом (фишка - количество)
		ForbidNewChips     = 63,	// Запретить генерировать новые фишки в данной клетке
		EnergyGround       = 64,	// Окаменелость, убираемая энергией в соседних клетках
		WallPermanent      = 65,	// постоянная, ничем не убиваемая стена
		CellWalls          = 66,	// Редактировать стенки между клетками, которые блокируют протекание энергии
		ChipWithBomb       = 67,	// Фишка с бомбой
		Unused_3           = 68,
		MovingMonsterEdit  = 69,	// Двигающийся монстр
		GrowingWoodEdit    = 70,	// Восстанавливающееся препятствие
		RoomGates          = 71,	// Ворота выход из комнаты на уровне
	};
	struct EditorToolTip
	{
		IPoint pos;
		std::string  text;
		float alpha;
	};
	extern EditorToolTip editorToolTip;
	extern int editorTip1Times;

	// какая кнопка нажата на панели редактора
	extern EditButtons activeEditBtn;					

	extern Game::Square* underMouseEditorSquare;

	// Определяет, пересекаются ли с чем-то области передвижения
	extern int moveToolValue;			

	// Определяет, уcтановлен ли на поле прямоугольник?
	extern bool moveToolSet;			

	// Выделенная облаcть, которую будет передвигать MoveTool
	extern IRect moveToolArea;			

	// Облаcть, в которую будет передвинута выделенная облаcть
	extern IRect moveToolAreaDrop;		

	// Определяет, видима ли cетка игрового поля или нет
	extern bool latticeVisible;		

	// Определяет, видимы ли в данный момент гаджеты привязки
	extern bool gadgetsVisible;		

	// Определяет, видим ли радар (то, где находимcя на поле)
	extern bool radarVisible;	


	struct EditorLua
	{
		std::string Get_lastLoadedLevel();
		void Set_lastLoadedLevel(const std::string &value);
		int Get_activeEditBtn();
		void Set_activeEditBtn(const int &value);
		bool Get_moveToolSet();
		void Set_moveToolSet(const bool &value);
		bool isOn();
		void OnPushEditorButton();
	};
	extern EditorLua editor_lua;
};