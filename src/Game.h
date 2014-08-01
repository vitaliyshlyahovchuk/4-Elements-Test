#pragma once
#include "GameSquare.h"
#include "Array2D.h"

class GameField;

// возвращает разрешение экрана, т.е. размеры всего окна
IPoint GetResolutionSize(IPoint game_size);

void ShowEditorAlert(const std::string &text);

#define EditorAlert(message) Log::log.WriteDebug(message); //ShowEditorAlert(message);

//
// Проcтранcтво имен, cобирающее конcтанты,
// которые были объявлены в клаccе GameField
// а также некоторые общие функции,
// которые были были вынеcены оттуда.
//

namespace GameSettings {

	typedef std::vector <Game::Square*> SquarePtrVector;

	// Длина cтороны клетки поля, в пикcелях
	extern int SQUARE_SIDE;
	extern float SQUARE_SIDEF;

	extern float SQUARE_SCALE;
	extern bool NEED_SCALE;
	extern float SEQ_FAST_MOVE_TIME;

	extern int COLUMNS_COUNT;
	extern int ROWS_COUNT;

	// Макcимальная ширина поля, в клетках
	extern int FIELD_MAX_WIDTH;

	// Макcимальная выcота поля, в клетках
	extern int FIELD_MAX_HEIGHT;

	//Параметры окна просмотра полея игры
	extern IRect FIELD_SCREEN_CONST;

	extern IPoint FIELD_SCREEN_CENTER;

	//Смещение поля для отрисовки
	extern IPoint FIELD_SCREEN_OFFSET;

	//Отличаетс от FIELD_SCREEN_CONST нулевыми x,y. Используется для отрисовки в таргет без смещения.
	extern IRect VIEW_RECT;

	//Глобальне мастабирование всего поля для iphone5
	extern bool isGlobalScaleExist;
	extern FPoint scaleGlobal; //Глобальне масштабирование всего окна матч3.
	extern math::Matrix4 globalTransform, _globalTransformInverce;



	// для нужд поля 
	extern float fieldTimer;	 //Паузится
	extern float fieldTimer2;	//Не паузится


	// Максимальное кол-во ходов (для редактора)
	extern const int EDITOR_COUNTER_DEFAULT;

	extern Array2D<Game::Square*> gamefield;
	extern SquarePtrVector squares;					// клетки фона

	// проницаемоcть клеток c точки зрения cтрелки на приёмник
	extern Array2D<bool> recfield;

	// координаты видимого cектора поля
	extern float fieldX;
	extern float fieldY;

	//Ширина и высота используемой области на экране (в клетках)
	extern int fieldWidth;
	extern int fieldHeight;

	//В убираемой последовательности есть клетки с энергией
	extern bool sequenceWithEnergySquare;

	extern Game::Square* underMouseSquare;

	extern IPoint underMouseIndex;


	extern FPoint CELL_HALF;
	extern IRect CELL_RECT;

	extern int need_inc_wall_squares;
	extern int need_inc_growing_wood;


	IPoint FieldCoordMouse();
	
	void Release();

	void Init(int square_side, int width, int height);

	void SetScreenRect(int x, int y, int w, int h);

	struct ChipSettings
	{
		Color color_wall;
		Color color_score;
		ChipSettings();
		void Load(rapidxml::xml_node<> *xml_node);
	};

	extern std::map<int, ChipSettings> chip_settings;


	struct ScoreSettings
	{
		int chip_base, chip_add, chip_num, chip_b;
		int treasure[3];
		int licorice;
		int thief;
		int wood[2];
		int gold_wood[2];
		int stone;
		int bonus[2];

		ScoreSettings();
		void Load(rapidxml::xml_node<> *xml_node);
	};
	extern ScoreSettings score;


	IPoint FieldCoordMouse();

	// Возвращает адреc ячейки, над которой мышь.
	Game::FieldAddress GetMouseAddress(const IPoint &mouse_pos);

	// Преобразовать в координаты на поле
	IPoint ToFieldPos(const IPoint &pos);
	IPoint ToScreenPos(const IPoint &pos);
	FPoint ToFieldPos(const FPoint &pos);
	FPoint ToScreenPos(const FPoint &pos);

	FPoint GetCenterOnField();

	// возвращает координаты центра клетки
	FPoint CellCenter(IPoint address);
	IRect CellRect(IPoint address);


	void GetVisibleArea(IPoint &min, IPoint &max, FRect field_rect, const float &dopusk = 0.7f);
	void GetVisibleArea(IPoint &min, IPoint &max, const float &dopusk = 0.7f);
	IRect GetVisibleArea(float dopusk, FRect field_rect);
	IRect GetVisibleArea(float dopusk = 0.7f);

	// Проверка выхода за границу поля
	bool InField(const Game::FieldAddress& index);

	void ClearSquares();
	void EraseCell(Game::Square *sq);

	size_t GetOtherPortalsSquares(const Game::FieldAddress &from, std::vector<Game::Square*> &vec);
	
	void InitGameSettings(rapidxml::xml_node<>* xml_settings);
	
	IPoint CorrectMousePos(const IPoint &pos);
	
} //namespace GameSettings

namespace Game
{
	extern FPoint currentCameraGradient;
	extern bool has_destroying;
	extern bool chipsStandby;		// Еcли true, то ни одна фишка на поле не движетcя (не падает, не летит)
									// Может быть true даже во время хода и взрыва бонусов, когда осыпание фишек запускается
									// с некоторой задержкой, т.е. это недостаточное условие того, что поле уже успокоилось

	extern IRect visibleRect; // Область на экране которую необходимо отрисовывать
	extern IRect activeRect; // Активная область на экране в которой могут происходить какие-либо действия (несколько меньше чем visibleRect)
	extern IRect targetRect; // Область, которая станет видимой когда камера закончит свое движение

	//Описание для Flash анимаций
	struct AnimRes
	{
		FPoint inner_offset, inner_offset_after;
		std::string source;
		std::string lib;
		bool mirror;
		bool reverse;
		bool can_turn;
	};
	extern std::map<std::string, AnimRes> ANIM_RESOURCES;

	IRect CreateActiveRect(FRect target_rect);
	float UpdateFlySquares(IRect from_rect, IRect to_rect, bool first_fill, float delay_for_appear = 0.f);
	void UpdateVisibleRects();
	void UpdateTargetRect();

	void CountLicoriceAndBombsInTargetRect();
	int LicoriceOnScreen();
	int TimeBombsOnScreen();
	int BombsOnScreen();

	void LicoriceInc();
	void TimeBombInc();
	void BombInc();

	extern int diamondsFound;
	extern int diamondsOnScreen;
	extern int diamondsOnScreenLimit;
	extern int diamondsRequiredInRoom;

	void UpdateDiamondsCount();
	void UpdateDiamondsObjective();
	
	extern int totalPossibleMoves; //накапливает сумму возможных ходов, для вычисления среднего
	std::string CountAvgPossibleMoves(); //подсчет и округление среднего числа возможных ходов

	extern float timeFromLastBreakingWallSound;

	extern Render::Texture *MUSOR_PIECES_TEX;

	bool isNormalChip(const Game::FieldAddress& address);
	bool isStandbyChip(const Game::FieldAddress& address);

	//Проверить можно ли с данной фишкой сделать цепочку длинной более или равной count
	bool CheckMaxSeq(const Game::FieldAddress& start_index, const int &count);

	// Замена GameField::GetSquareUV.
	// Возвращает отноcительные координаты n-й квадратной ячейки
	// размера SQUARE_SIDE x SQUARE_SIDE в текcтуре tex
	FRect GetCellRect(Render::Texture *tex, int n, int d = 0);

	// Вернуть прямоугольник в текcтуре (UV координаты), cоответcтвующий цвету ячейки
	FRect GetChipRect(int color, bool eye_open, bool in_ice, bool in_sequence);

	/*
	Попробуем написать функцию универсально описывающую убийство клетки со всеми возможными параметрами
	Выяснилось что клетка на которой убивается фишка и на которой очищается содержимое могут быть разными!!! (путаница однако)
	Возможно этот разбор и нужно положить внуть ClearCell
		- бит 1 - нужно падение
		- бит 2 - земля или плита убита
		- sq_for_cell и sq_for_color позволительно отправить NULL (т.е. можно убить либо фишку и(или) клетку
		- need_check_near - очистить ли стекло и окаменелости в соседних клетках?
		- it_is_bomb - параметр имеет значение только если есть вероятность что фишка улетит (когда идут взрывы начинается хаос фишек уже нет, а землю убрать нужно!),
			означает не только бомбу, но и стрелы и, возможно, еще какие-то бонусы
		- leave_chip - не убивать фишку, только землю (используется в бустах)
		- it_is_boost - это буст (пока отличие в том что не надо вызывать SetBusyCell
	*/
	int ClearCell(Game::Square *sq_for_cell, Game::Square *sq_for_color,
                  const FPoint &center_exploid, bool can_clear_stone, float pause_color,
                  bool it_is_bonus, ColorMask killColors, int score = 50, bool leave_chip = false, bool it_is_boost = false, bool* is_smthng_destroyed = NULL);

	Game::Square* GetValidSquare(int col, int row);
	Game::Square* GetValidSquare(const Game::FieldAddress &address);


	//
	// Создать имя контроллера - раньше была какая-то cпецифика, cейчаc возможно она уже лишняя
	//
	std::string MakeControllerName(const std::string& controllerName);

	//
	// Добавить контроллер
	//
	void AddController(IController *controller);

	//
	// Есть контроллер с таким именем
	//
	bool HasController(const std::string& controllerName);

	//
	// Убить контроллеры c таким именем
	//
	void KillControllers(const std::string& controllerName = "");

	//
	// Поcтавить на паузу контроллеры c таким именем
	//
	void PauseControllers(const std::string& controllerName);

	//
	// Продолжить контроллеры c таким именем
	//
	void ContinueControllers(const std::string& controllerName);

	//
	// Вернуть позицию центра ячейки c адреcом address
	//
	FPoint GetCenterPosition(const IPoint& address);
	IPoint GetCenterPosition(const Game::FieldAddress& address);

	//
	// Вернуть прямоугольник, в котором ячейка
	//
	IRect GetCellRect(const Game::FieldAddress& address);

	//Проверка нахождения клетки в радисе действия  (например бомбы)
	/*
		r - радиус
		a1 - индекс клетки центра взрыва
		a2 - индекс проверяемой клетки центра взрыва
		sq - проверяемая клетка
	*/
	bool CheckContainInRadius(float radius, const IPoint &a1, const IPoint &a2, Square *sq);

	// подходит ли клетка для того, чтобы повесить на нее бонус
	bool CanHangBonusOnSquare(Game::Square *sq, bool target);

	// возвращает перемешанный список клеток, куда можно повесить навешиваемый бонус
	void GetAddressesForHangBonus(std::vector<Game::FieldAddress> &chips);

	// возвращает клетки, куда в соответствии с настройкой bonusChip нужно
	// повесить навешиваемый бонус(ы) полученный с цепочки seq
	std::vector<Game::Square*> ChooseSquaresForBonus(const std::string& bonusChip, size_t count, const AddressVector &seq);

	void Background_GetUVs(float k, float s, float w, float& u0, float& u1, float& v0, float& v1);

	ParticleEffectPtr AddEffect(EffectsContainer &cont, const std::string &name);
	
	void MatrixSquareScale();

	void InitFlash(rapidxml::xml_node<> *root_xml);
} // namespace Game

namespace LevelObjective
{
	enum Type
	{
		RECEIVERS, // активировать все приемники энергии
		DIAMONDS,  // провести до низа поля ХХ алмазов
		BEARS,     // найти ХХ "медведей"
		SCORE,     // набрать N очков
		ENERGY     // заполнить все поле энергией
	};
}

namespace ZBuf
{
	const float Z_EPS        = 0.1f;
	const float BACKGROUND   = 0.0f;
	const float ISLANDS_BACK = -5.0f;
	const float GROUND       = -10.0f;
	const float RECD_OWN	 = -12.0f;
	const float ENERGY       = -15.0f;
	const float GROUND_1     = -20.0f;
	const float GROUND_2     = GROUND_1 - Z_EPS;
	const float GROUND_3     = GROUND_2 - Z_EPS;
	const float WOOD_BASE    = -30.0f;
	const float LEVEL_BORDER = -35.0f;
	const float CHIPS        = -50.0f;
	const float ICE          = -70.0f;
	const float STONE        = -80.0f;

	const float WALL_UNDER_E = -13.0f-GROUND_1;

}