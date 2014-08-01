#include "stdafx.h"
#include "Game.h"

#include "GameFieldAddress.h"
#include "GameField.h"
#include "GameInfo.h"
#include "Match3Gadgets.h"
#include "SomeOperators.h"
#include "MyApplication.h"
#include "Match3Spirit.h"
#include "EnergyReceivers.h"
#include "LockBarriers.h"
#include "RoomGates.h"
#include "EditorUtils.h"

#ifdef ENGINE_TARGET_IPHONE
#include "AppSettings.h"
#endif

void ShowEditorAlert(const std::string &text)
{
#if defined(ENGINE_TARGET_WIN32)
	Log::log.WriteInfo(text);
	MessageBox(Core::appInstance->getMainWindow()->hWnd, text.c_str(), "EditorAlert", MB_ICONINFORMATION);
#elif defined(ENGINE_TARGET_IPHONE)

#elif defined(ENGINE_TARGET_ANDROID)

#else

#endif
}

namespace GameSettings
{
	// Длина cтороны клетки поля, в пикcелях
	int SQUARE_SIDE = 80;
	float SQUARE_SIDEF = 80.0f;

	float SQUARE_SCALE = 1.f;
	bool NEED_SCALE = false;

	int COLUMNS_COUNT = 8;
	int ROWS_COUNT = 10;

	float SEQ_FAST_MOVE_TIME = 0.2f;

	// Макcимальная ширина поля, в клетках
	int FIELD_MAX_WIDTH = 0;

	// Макcимальная выcота поля, в клетках
	int FIELD_MAX_HEIGHT = 0;

	//Рабочие параметры экрана игры (минус большие панельки)
	IRect FIELD_SCREEN_CONST = IRect(0,0,0,0);

	IPoint FIELD_SCREEN_CENTER = IPoint(0,0);

	IRect VIEW_RECT = FIELD_SCREEN_CONST;

	IPoint FIELD_SCREEN_OFFSET = IPoint(0,0);

	// Максимальное кол-во ходов (для редактора)
	const int EDITOR_COUNTER_DEFAULT = 40;


	float fieldTimer = 0.f;
	float fieldTimer2 = 0.f;

	int dx8_ccw[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	int dy8_ccw[8] = {0, 1, 1, 1, 0, -1, -1, -1};


	bool isGlobalScaleExist = false;
	FPoint scaleGlobal(1.f, 1.f);
	math::Matrix4 globalTransform = math::Matrix4::Identity;
	math::Matrix4 globalTransformInverce = math::Matrix4::Identity;


	SquarePtrVector squares;
	Array2D<Game::Square*> gamefield = Array2D<Game::Square*>();

	// Клетки по которым может течь энергия!
	Array2D<bool> recfield = Array2D<bool>(); 

	int need_inc_wall_squares = 0;

	int need_inc_growing_wood = 0; //нужно ли ростить восстанавливающееся препятствие

	//Используется как в игре так и в редакторе для вывода значений клетки
	IPoint underMouseIndex = IPoint(0,0);
	FPoint CELL_HALF;
	IRect CELL_RECT;

	ChipSettings::ChipSettings()
		: color_wall(Color::WHITE)
		, color_score(Color::WHITE)
	{
	}
	
	void ChipSettings::Load(rapidxml::xml_node<> *xml_node)
	{
		color_wall = Color(Xml::GetStringAttribute(xml_node, "color_wall"));
		color_score = Color(Xml::GetStringAttribute(xml_node, "color_score"));
	}
	
	std::map<int, ChipSettings> chip_settings;

	ScoreSettings::ScoreSettings()
		: chip_base(200)
		, chip_add(5)
		, chip_num(3)
		, chip_b(50)
		, licorice(50)
		, thief(50)
		, stone(50)
	{
		treasure[0] = 100; treasure[1] = 500; treasure[2] = 2000;
		bonus[0] = 500; bonus[1] = 2500;
		wood[0] = 50; wood[1] = 100;
		gold_wood[0] = 1000; gold_wood[1] = 3000;
	}

	void ScoreSettings::Load(rapidxml::xml_node<> *xml_node)
	{
		if(xml_node)
		{
			rapidxml::xml_node<> *node = xml_node->first_node("Treasure");
			treasure[0] = Xml::GetIntAttributeOrDef(node, "level1", 100);
			treasure[1] = Xml::GetIntAttributeOrDef(node, "level2", 500);
			treasure[2] = Xml::GetIntAttributeOrDef(node, "level3", 2000);

			node = xml_node->first_node("Chip");
			chip_base = Xml::GetIntAttributeOrDef(node, "base", 200);
			chip_add = Xml::GetIntAttributeOrDef(node, "add", 50);
			chip_num = Xml::GetIntAttributeOrDef(node, "num", 3);
			chip_b = Xml::GetIntAttributeOrDef(node, "bonus", 50);

			node = xml_node->first_node("SpecialChip");
			licorice = Xml::GetIntAttributeOrDef(node, "licorice", 50);
			thief = Xml::GetIntAttributeOrDef(node, "thief", 50);

			node = xml_node->first_node("Square");
			wood[0] = Xml::GetIntAttributeOrDef(node, "wood", 50);
			wood[1] = Xml::GetIntAttributeOrDef(node, "wood_final", 100);
			gold_wood[0] = Xml::GetIntAttributeOrDef(node, "gold_wood", 1000);
			gold_wood[1] = Xml::GetIntAttributeOrDef(node, "gold_wood_final", 3000);
			stone = Xml::GetIntAttributeOrDef(node, "stone", 50);

			node = xml_node->first_node("Bonus");
			bonus[0] = Xml::GetIntAttributeOrDef(node, "score", 500);
			bonus[1] = Xml::GetIntAttributeOrDef(node, "endGame", 2500);
		}
	}
	ScoreSettings score;

	void Init(int square_side, int width, int height)
	{
		SQUARE_SIDE = square_side;
		SQUARE_SIDEF = SQUARE_SIDE + 0.f;

		CELL_HALF.x = SQUARE_SIDE/2.f;
		CELL_HALF.y = SQUARE_SIDE/2.f;

		CELL_RECT = IRect(0, 0, SQUARE_SIDE, SQUARE_SIDE);

		gamefield.Release();

		Game::Square::DRAW_RECT = IRect(0,0, SQUARE_SIDE,SQUARE_SIDE);
		Game::ChipColor::DRAW_RECT = IRect(0,0, SQUARE_SIDE, int(SQUARE_SIDE*100.f/80.f));

		if(GameSettings::isGlobalScaleExist)
		{
			float h = SQUARE_SIDE*100.f/80.f;
			Game::ChipColor::DRAW_RECT = IRect(0, int(h/2.f - h/2.f/GameSettings::scaleGlobal.y), SQUARE_SIDE, int(h/GameSettings::scaleGlobal.y));
		}
		Game::ChipColor::DRAW_FRECT = FRect(Game::ChipColor::DRAW_RECT);

		FIELD_MAX_WIDTH = width;
		FIELD_MAX_HEIGHT = height;

		Game::bufSquare = new Game::Square(Game::FieldAddress(-1,-1));
		Game::bufSquare->SetWall(0);
		Game::bufSquare->SetFake(true);
		Game::bufSquare->SetEnergyChecked();

		Game::bufSquareNoChip = new Game::Square(Game::FieldAddress(-1,-1));
		Game::bufSquareNoChip->SetWall(0);
		Game::bufSquareNoChip->SetFake(true);
		Game::bufSquareNoChip->SetEnergyChecked();

		Game::bufSquareNoLicorice = new Game::Square(Game::FieldAddress(-1,-1));
		Game::bufSquareNoLicorice->SetWall(0);
		Game::bufSquareNoLicorice->SetFake(true);
		Game::bufSquareNoLicorice->SetEnergyChecked();

		Game::bufSquareNoTimeBomb = new Game::Square(Game::FieldAddress(-1,-1));
		Game::bufSquareNoTimeBomb->SetWall(0);
		Game::bufSquareNoTimeBomb->SetFake(true);
		Game::bufSquareNoTimeBomb->SetEnergyChecked();

		Game::bufSquareNoBomb = new Game::Square(Game::FieldAddress(-1,-1));
		Game::bufSquareNoBomb->SetWall(0);
		Game::bufSquareNoBomb->SetFake(true);
		Game::bufSquareNoBomb->SetEnergyChecked();

		gamefield.Init(FIELD_MAX_HEIGHT + 2, FIELD_MAX_WIDTH + 2, Game::bufSquare, 1, 1); 
		recfield.Init(GameSettings::FIELD_MAX_WIDTH + 2, GameSettings::FIELD_MAX_HEIGHT + 2, false, 1, 1);
	}

	void Release()
	{
		gamefield.Release();
		recfield.Release();

		if(Game::bufSquare)
		{
			delete Game::bufSquare;
			Game::bufSquare = NULL;
		}

		if(Game::bufSquareNoChip)
		{
			delete Game::bufSquareNoChip;
			Game::bufSquareNoChip = NULL;
		}

		if(Game::bufSquareNoLicorice)
		{
			delete Game::bufSquareNoLicorice;
			Game::bufSquareNoLicorice = NULL;
		}

		if(Game::bufSquareNoTimeBomb)
		{
			delete Game::bufSquareNoTimeBomb;
			Game::bufSquareNoTimeBomb = NULL;
		}

		if(Game::bufSquareNoBomb)
		{
			delete Game::bufSquareNoBomb;
			Game::bufSquareNoBomb = NULL;
		}
	}

	void SetScreenRect(int x, int y, int w, int h)
	{
		FIELD_SCREEN_CONST.x = x;
		FIELD_SCREEN_CONST.y = y;
		FIELD_SCREEN_CONST.width = w;
		FIELD_SCREEN_CONST.height = h;

		{
			const int h_down_panel = 84;
			const int h_up_panel = 106;
			const int h_game_cells = GameSettings::ROWS_COUNT*GameSettings::SQUARE_SIDE;
			const int h_border_down = 38;
			const int h_border_up = 6;
			int h_content_full = h_down_panel + h_border_down + h_game_cells + h_border_up + h_up_panel;
			int h_space_full = h - h_content_full;
			int h_space = h_space_full/3;
			GameSettings::FIELD_SCREEN_CENTER.x = GameSettings::FIELD_SCREEN_CONST.width/2;
			GameSettings::FIELD_SCREEN_CENTER.y = h_down_panel + h_space + h_border_down + h_game_cells/2;
			gameInfo.setGlobalInt("InterfacesGameSpace", h_space);
		}
		//GameSettings::FIELD_SCREEN_CENTER = IPoint(GameSettings::FIELD_SCREEN_CONST.width/2, (100.f + GameSettings::FIELD_SCREEN_CONST.height - 150.f)/2.f);

		FIELD_SCREEN_OFFSET = FIELD_SCREEN_CONST.LeftBottom();

		VIEW_RECT = FIELD_SCREEN_CONST.MovedTo(IPoint(0,0));
	}


	float fieldX = 0.f;
	float fieldY = 0.f;

	//Ширина и высота используемой области на экране (в клетках)
	int fieldWidth;
	int fieldHeight;

	bool sequenceWithEnergySquare = false;

	IPoint FieldCoordMouse()
	{
		return FPoint(fieldX, fieldY).Rounded() - FIELD_SCREEN_OFFSET;
	}
	
	Game::FieldAddress GetMouseAddress(const IPoint &mouse_pos)
	{
		int col = (mouse_pos.x + FieldCoordMouse().x) / SQUARE_SIDE;
		int row = (mouse_pos.y + FieldCoordMouse().y) / SQUARE_SIDE;
		return Game::FieldAddress(col, row);
	}


	IPoint ToFieldPos(const IPoint &pos)
	{
		return pos + FieldCoordMouse();
	}

	IPoint ToScreenPos(const IPoint &pos)
	{
		return pos - FieldCoordMouse();
	}

	FPoint ToFieldPos(const FPoint &pos)
	{
		return pos + FPoint(fieldX, fieldY) - FPoint(FIELD_SCREEN_OFFSET);
	}

	FPoint ToScreenPos(const FPoint &pos)
	{
		return pos - FPoint(fieldX, fieldY) + FPoint(FIELD_SCREEN_OFFSET);
	}

	FPoint GetCenterOnField()
	{
		return FPoint(GameSettings::fieldX + GameSettings::FIELD_SCREEN_CENTER.x, GameSettings::fieldY + GameSettings::FIELD_SCREEN_CENTER.y);
	}

	IRect CellRect(IPoint address)
	{
		IPoint pos = address * GameSettings::SQUARE_SIDE;
		return CELL_RECT.MovedTo(pos);
	}
	
	FPoint CellCenter(IPoint address)
	{
		return FPoint(address) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;
	}


	// Координаты видимой облаcти в координатах Game::Square
	void GetVisibleArea(IPoint &min, IPoint &max, FRect field_rect, const float &dopusk)
	{
		min.x = int( field_rect.xStart / GameSettings::SQUARE_SIDE + dopusk); //Должно торчать больше 0.7 части клетки 
		min.y = int( field_rect.yStart / GameSettings::SQUARE_SIDE + dopusk);

		max.x = int( (field_rect.xEnd -1) / GameSettings::SQUARE_SIDE - dopusk);
		max.y = int( (field_rect.yEnd -1) / GameSettings::SQUARE_SIDE - dopusk);

		// Проверяем, не оказалиcь ли вне границ маccива...
		if (max.x > GameSettings::FIELD_MAX_WIDTH - 1)
		{
			max.x = GameSettings::FIELD_MAX_WIDTH - 1;
		}
		if (max.y > GameSettings::FIELD_MAX_HEIGHT - 1)
		{
			max.y = GameSettings::FIELD_MAX_HEIGHT - 1;
		}
		if (min.x < 0)
		{
			min.x = 0;
		}
		if (min.y < 0)
		{
			min.y = 0;
		}
	}

	void GetVisibleArea(IPoint &min, IPoint &max, const float &dopusk)
	{
		GetVisibleArea(min, max, FRect(GameSettings::fieldX, GameSettings::fieldX + GameSettings::FIELD_SCREEN_CONST.width,  GameSettings::fieldY, GameSettings::fieldY + GameSettings::FIELD_SCREEN_CONST.height), dopusk);			
	}
	

	IRect GetVisibleArea(float dopusk, FRect field_rect)
	{
		IPoint min, max;
		GetVisibleArea(min, max, field_rect, dopusk);
		return IRect(min.x, min.y, max.x - min.x + 1, max.y - min.y + 1);
	}

	IRect GetVisibleArea(float dopusk)
	{
		IPoint min, max;
		GetVisibleArea(min, max, FRect(GameSettings::fieldX, GameSettings::fieldX + GameSettings::FIELD_SCREEN_CONST.width,  GameSettings::fieldY, GameSettings::fieldY + GameSettings::FIELD_SCREEN_CONST.height), dopusk);
		return IRect(min.x, min.y, max.x - min.x + 1, max.y - min.y + 1);
	}


	bool InField(const Game::FieldAddress& index) 
	{
		return index.GetCol() >= 0 &&
			index.GetCol() < GameSettings::fieldWidth &&
			index.GetRow() >= 0 &&
			index.GetRow() < GameSettings::fieldHeight;
	}

	void ClearSquares()
	{
		for (int j = 0; j < GameSettings::FIELD_MAX_HEIGHT + 2; j++)
		for (int i = 0; i < GameSettings::FIELD_MAX_WIDTH + 2; i++)
		{
			GameSettings::gamefield[i][j] = Game::bufSquare;
			GameSettings::recfield[i][j] = false;
		}

		for(Game::Square *sq : squares)
		{
			Assert( !Game::isBuffer(sq) );
			delete sq;
		}
		GameSettings::squares.clear();
		GameSettings::squares.shrink_to_fit();
	}

	void EraseCell(Game::Square *sq)
	{
		auto itr = std::find(squares.begin(), squares.end(), sq);
		if( itr != squares.end() )
			squares.erase(itr);

		GameField::gameField->UpdateGameField(false);

		GameField::gameField->_levelScopeUpdated = false;
		GameField::gameField->_levelScopeUpdateTime = 0.0f;
	}

	size_t GetOtherPortalsSquares(const Game::FieldAddress &from, std::vector<Game::Square*> &vec)
	{	
		vec.clear();
		const std::string portal_id = GameSettings::gamefield[from]->portalEnergyId;
		if(portal_id.empty())
		{
			return 0;
		}
		size_t count = GameSettings::squares.size();
		for(size_t i = 0; i < count; i++)
		{
			Game::Square *sq = GameSettings::squares[i];
			if(sq->address != from && sq->portalEnergy && sq->portalEnergyId == portal_id)
			{
				vec.push_back(sq);
			}	
		}		
		return vec.size();
	}

	std::string GetDeviceId(IPoint size)
	{
		float proportions = (size.x + 0.f)/size.y;
		if(proportions > 0.74)
		{
			//iPad
			//IPoint(768, 1024); //0.74
			return "ipad";
		}
		if(proportions > 0.6)
		{
			//iPhone
			//IPoint(640, 960); //0.66
			return "iphone";
		}
		//Дальше ближе только iphone5
		//if(proportions > 0.56)
		//{
			//iPhone 5
			//IPoint(640, 1136); //0.56
			return "iphone5";
		//}
		//return "";
	}
	
	void InitGameSettings(rapidxml::xml_node<>* xml_settings)
	{
		IPoint game_size;
		game_size.x = Xml::GetIntAttributeOrDef(xml_settings, "game_width", 640);
		game_size.y = Xml::GetIntAttributeOrDef(xml_settings, "game_height", 960);

		IPoint device_size = GetResolutionSize(game_size);
		game_size.x = device_size.x; //Ширина - на весь экран

		bool is_retina = false;
		std::string device_id = GetDeviceId(device_size);

		gameInfo.setGlobalString("device_id", device_id);
		
		MyApplication::GAME_WIDTH = device_size.x;
		MyApplication::GAME_HEIGHT = device_size.y;

		//Размер поля на всех устройствах теперь одинаковый!
		int game_square_side = Xml::GetIntAttributeOrDef(xml_settings, "game_square_side", 80);
		//if(device_id == "ipad")
		//{
		//	game_square_side = 96;
		//}
		SQUARE_SCALE = game_square_side/80.f;
		NEED_SCALE = (SQUARE_SCALE != 1.f);
		//По ширине на весь экран
		//По высоте - rows клеток
		GameSettings::COLUMNS_COUNT = Xml::GetIntAttributeOrDef(xml_settings, "columns", 8); 
		GameSettings::ROWS_COUNT = Xml::GetIntAttributeOrDef(xml_settings, "rows", 10);

		Init(game_square_side, 200, 200);
		SetScreenRect(0.f, 0.f, device_size.x, device_size.y);
	}



	IPoint CorrectMousePos(const IPoint &mouse_pos)
	{
		if(!isGlobalScaleExist)
		{
			return mouse_pos;
		}
		math::Vector3 pos = mouse_pos;
		pos = pos.TransformCoord(globalTransformInverce);
		return FPoint(pos.x, pos.y).Rounded();
	}

} //namespace GameSettings

namespace Game 
{
	FPoint currentCameraGradient(1.f, 0.f);

	bool has_destroying = false;	//Клетки разрушаются
	bool chipsStandby = false;
	IRect visibleRect;
	IRect activeRect;
	IRect targetRect;

	int licoriceOnScreen = 0;
	int timeBombsOnScreen = 0;
	int bombsOnScreen = 0; //сколько сейчас бомбочек на экране

	int totalPossibleMoves; //накапливает сумму возможных ходов, для вычисления среднего

	std::map<std::string, AnimRes> ANIM_RESOURCES;

	//подсчет и округление среднего числа возможных ходов
	std::string CountAvgPossibleMoves()
	{
		int moves = utils::lexical_cast<int>(playerStatistic.GetValue("SpentMoves"));
		if (moves == 0)
		{
			return "0";
		} else {
			float avg = Game::totalPossibleMoves / static_cast<float> (moves);

			std::ostringstream ss;
			ss << std::setprecision(5) << avg;
			return ss.str();
		}
	}

	IRect CreateActiveRect(FRect target_rect)
	{
		IRect active_rect;
		FPoint center = target_rect.LeftBottom() + GameSettings::FIELD_SCREEN_CENTER;
		IPoint center_cell(int(center.x/GameSettings::SQUARE_SIDEF), int(center.y/GameSettings::SQUARE_SIDEF));
		if(abs(center_cell.x*GameSettings::SQUARE_SIDEF - center.x) > GameSettings::SQUARE_SIDEF*0.25f)
		{ //Неточное попадание по х
			active_rect.x = center_cell.x - GameSettings::COLUMNS_COUNT/2 + 1;
			active_rect.width = GameSettings::COLUMNS_COUNT -1;
		}else{
			active_rect.x = center_cell.x - GameSettings::COLUMNS_COUNT/2;
			active_rect.width = GameSettings::COLUMNS_COUNT;
		}
		if(abs(center_cell.y*GameSettings::SQUARE_SIDEF - center.y) > GameSettings::SQUARE_SIDEF*0.25f)
		{ //Неточное попадание по y
			active_rect.y = center_cell.y - GameSettings::ROWS_COUNT/2 + 1;
			active_rect.height = GameSettings::ROWS_COUNT -1;
		}else{
			active_rect.y = center_cell.y - GameSettings::ROWS_COUNT/2;
			active_rect.height = GameSettings::ROWS_COUNT;
		}		
		return active_rect;
	}

	void UpdateVisibleRects()
	{
		Game::visibleRect = GameSettings::GetVisibleArea(-0.6f); //Видимая часть рисуется с допуском наружу
		IRect new_active_rect = CreateActiveRect(FRect(GameSettings::fieldX, GameSettings::fieldX + GameSettings::VIEW_RECT.width, GameSettings::fieldY, GameSettings::fieldY + GameSettings::VIEW_RECT.height));
		if(new_active_rect.x != Game::activeRect.x || new_active_rect.y != Game::activeRect.y)
		{
			Game::activeRect = new_active_rect;
			GameField::Get()->levelBorder.Calculate();
		}else{
			Game::activeRect = new_active_rect;
		}
		//Устанавливаем новую видимую область для отрисовки динамических элементов
		if(EditorUtils::editor)
		{
			Place2D::SetViewRect(Game::visibleRect);
		}else{
			Place2D::SetViewRect(IRect(Game::activeRect.x -1, Game::activeRect.y -1, Game::activeRect.width + 1, Game::activeRect.height + 1));
		}
	}

	bool SortByEnergySourceDistance(Game::Square *sq1, Game::Square *sq2)
	{
		float dist1 = Gadgets::squareDist[sq1->address];
		float dist2 = Gadgets::squareDist[sq2->address];
		return dist1 > dist2;
	}


	float UpdateFlySquares(IRect from_rect, IRect to_rect, bool first_fill, float delay_for_appear)
	{
		float pauseWhileSquareFlyHiding = 0.f;
		std::list<Game::Square*> hidding_squares, appearing_squares;
		{
			for(int x = from_rect.LeftBottom().x; x <= from_rect.RightTop().x; x++)
			{
				for(int y = from_rect.LeftBottom().y; y <= from_rect.RightTop().y; y++)
				{
					IPoint index(x, y);
					if(!to_rect.Contains(index))
					{
						Game::Square *sq = GameSettings::gamefield[index];
						if(sq->IsFlyType(Game::Square::FLY_STAY))
						{
							hidding_squares.push_back(sq);
						}
					}
				}		
			}
			hidding_squares.sort(SortByEnergySourceDistance);
			float pause = 0.001f;
			std::vector<float> pauses(hidding_squares.size(), 0.f);
			float FLY_SQ_RAND_PAUSE_HIDE_DOWN = gameInfo.getConstFloat("FLY_SQ_RAND_PAUSE_HIDE_DOWN", 0.f);
			float FLY_SQ_RAND_PAUSE_HIDE_UP = gameInfo.getConstFloat("FLY_SQ_RAND_PAUSE_HIDE_UP", 0.3f);


			for(float &p: pauses)
			{
				pauseWhileSquareFlyHiding = pause;
				p = pause;
				pause += math::random(FLY_SQ_RAND_PAUSE_HIDE_DOWN, FLY_SQ_RAND_PAUSE_HIDE_UP);
			}
			pauseWhileSquareFlyHiding += Game::Square::FLY_SQ_DELAY_FOR_MOVE_BEFORE_LAST_HIDEN;
			//float add_pause = 0.f;
			//if(pauseWhileSquareFlyHiding < 0.f)
			//{
			//	//Паузы для появления не хватает, делаем длиннее задержку исчезновения
			//	add_pause = -pauseWhileSquareFlyHiding;
			//	pauseWhileSquareFlyHiding = 0.f;
			//}
			size_t i = 0;
			for(auto sq : hidding_squares)
			{
				sq->SetFlyType(Game::Square::FLY_HIDING, false);
				sq->SetFlyPause(pauses[i]);
				i++;
			}
		}
		{
			for (int x = to_rect.LeftBottom().x; x <= to_rect.RightTop().x; x++)
			{
				for (int y = to_rect.LeftBottom().y; y <= to_rect.RightTop().y; y++)
				{	
					Game::Square* sq = GameSettings::gamefield[x+1][y+1];
					if(!Game::isVisible(sq))
					{
						continue;
					}
					if(sq->IsFlyType(Game::Square::FLY_NO_DRAW))
					{
						appearing_squares.push_back(sq);
					}
				}
			}
			float pause = delay_for_appear + 0.001f;
			float FLY_SQ_RAND_PAUSE_APPEAR_DOWN = gameInfo.getConstFloat("FLY_SQ_RAND_PAUSE_APPEAR_DOWN", 0.f);
			float FLY_SQ_RAND_PAUSE_APPEAR_UP = gameInfo.getConstFloat("FLY_SQ_RAND_PAUSE_APPEAR_UP", 0.3f);
			appearing_squares.sort(SortByEnergySourceDistance);
			for(auto sq : appearing_squares)
			{
				sq->SetFlyType(Game::Square::FLY_APPEARING, first_fill);
				sq->SetFlyPause(pause);
				pause += math::random(FLY_SQ_RAND_PAUSE_APPEAR_DOWN, FLY_SQ_RAND_PAUSE_APPEAR_UP);
			}
		}	
		return pauseWhileSquareFlyHiding;
	}

	void UpdateTargetRect()
	{
		FPoint tmp(GameSettings::fieldX, GameSettings::fieldY);
		GameSettings::fieldX = GameField::Get()->_destFieldPos.x + 0.f;
		GameSettings::fieldY = GameField::Get()->_destFieldPos.y + 0.f;
		Game::targetRect = GameSettings::GetVisibleArea(-0.6f);
		GameSettings::fieldX = tmp.x;
		GameSettings::fieldY = tmp.y;
	}

	//считает лакриц, тайм-бом и навешанных бомб на экране
	void CountLicoriceAndBombsInTargetRect()
	{
		licoriceOnScreen = 0;
		timeBombsOnScreen = 0;
		bombsOnScreen = 0;

		//принято решение считать не на экране а вообще
		for(GameSettings::SquarePtrVector::iterator itr = GameSettings::squares.begin(); itr != GameSettings::squares.end(); ++itr)
		{
			Game::Square *sq=*itr;
				if( sq->GetChip().IsLicorice() ) {
					++licoriceOnScreen;
				} else if( sq->GetChip().IsTimeBomb() ) {
					++timeBombsOnScreen;
				} else	if (sq->GetChip().HasHangBomb())
					++bombsOnScreen;
		}
	}

	int diamondsFound = 0;
	int diamondsOnScreen = 0;
	int diamondsOnScreenLimit = 0;
	int diamondsRequiredInRoom = 0;

	void UpdateDiamondsCount()
	{
		diamondsOnScreen = 0;
		IPoint offset = Game::activeRect.LeftBottom();
		for(int x = 0; x < Game::activeRect.width; x++)
		{
			for(int y = 0; y < Game::activeRect.height; y++)
			{
				Game::FieldAddress fa(offset.x + x, offset.y + y);
				Game::Square *sq = GameSettings::gamefield[fa];
				if( sq->GetChip().GetType() == Game::ChipColor::DIAMOND )
					++diamondsOnScreen;
			}
		}

		diamondsOnScreenLimit = diamondsOnScreen;

		UpdateDiamondsObjective();
	}

	void UpdateDiamondsObjective()
	{
		Game::DestroyChipsOrder *order = Game::Orders::GetDiamondsOrderOnScreen();
		diamondsRequiredInRoom = order ? (order->GetCount() - order->GetCountKilled()) : (Gadgets::levelSettings.getInt("LevelObjectiveAmount") - diamondsFound);
	}

	int LicoriceOnScreen()
	{
		return licoriceOnScreen;
	}

	int TimeBombsOnScreen()
	{
		return timeBombsOnScreen;
	}

	int BombsOnScreen()
	{
		return bombsOnScreen;
	}

	void LicoriceInc()
	{
		licoriceOnScreen++;
	}

	void TimeBombInc()
	{
		timeBombsOnScreen++;
	}

	void BombInc()
	{
		bombsOnScreen++;
	}

	Render::Texture *MUSOR_PIECES_TEX = NULL;

	float timeFromLastBreakingWallSound = 0.f;

	bool isStandbyChip(const Game::FieldAddress& index)
	{
		if (GameSettings::InField(index)) {
			return GameSettings::gamefield[index]->IsStandbyState();
		} else {
			return false;
		}
	}

	bool isNormalChip(const Game::FieldAddress& index)
	{
		if (GameSettings::InField(index)) 
		{
			Game::Square *s = GameSettings::gamefield[index];
			return !isBuffer(s) && s->IsNormal() && !s->IsFake() && s->GetChip().IsExist();
		}
		return false;
	}

	FRect GetCellRect(Render::Texture *tex, int n, int d) 
	{
		IRect rectBitmap = tex->getBitmapRect();
		const int SIDE_ON_TEXTURE = 44;
		int numCols = rectBitmap.width / SIDE_ON_TEXTURE;
		int x = n % numCols;
		int y = n / numCols;
		float u1 = float (x * SIDE_ON_TEXTURE + d);
		float v1 = float (rectBitmap.height - (y + 1) * SIDE_ON_TEXTURE + d);
		float u2 = u1 + float (SIDE_ON_TEXTURE - d * 2);
		float v2 = v1 + float (SIDE_ON_TEXTURE - d * 2);
		FRect rect(0,1,0,1);
		FRect uv(u1, u2, v1, v2);
		tex->TranslateUV(rect, uv);

		return uv;
	}

	const float PERCENT_WIDTH = 80.f/720.f; //Относительный размер фишки на текстуре. Одинаковый и для ретины и не для ретины.
	const float PERCENT_HEIGHT = 100.f/700.f;

	FRect GetChipRect(int color, bool eye_open, bool in_ice, bool in_sequence)
	{
		float u1 = (float)(color % 9);
		float v2 = (float)(color / 9);
		if(in_ice)
		{ //Фишка во льду
			v2 += 3.f;
		}
		if(in_sequence)
		{
			//Выделяемая цепочка
			v2 += 2.f;
		}else if(eye_open)
		{
			//С открытыми глазами
			v2 += 1.f;
		}
		float u2 = u1 + 1.f;
		float v1 = v2 + 1.f;

		u1 *= PERCENT_WIDTH;
		u2 *= PERCENT_WIDTH;
		v1 = 1.f - v1*PERCENT_HEIGHT;
		v2 = 1.f - v2*PERCENT_HEIGHT;
		return FRect(u1, u2, v1, v2);
	}

	std::string MakeControllerName(const std::string& controllerName) {
		return controllerName;
	}

	void AddController(IController *controller) {
		Core::controllerKernel.addController(controller);
	}

	bool HasController(const std::string& controllerName) {
		return Core::controllerKernel.hasController(controllerName);
	}

	bool ClearNear(Game::Square *sq, float pause_color)
	{
		if(!Game::isVisible(sq))
		{
			return false;
		}
        
		if(!Game::activeRect.Contains(sq->address.ToPoint()))
		{
			return false;
		}
        
		if(sq->GetChip().IsLock())
		{
			return false;
		}
        
        bool returnValue = false;
        
        if (sq->GetWood() > 0)
        {
            returnValue = true;
        }
        
		// проверка ToBeDestroyed для того, чтобы не убивать два раза одну и ту же клетку бонусами и цепочкой,
		if(!sq->IsBusyNear() && !sq->ToBeDestroyed() && sq->KillSquareNear(pause_color))
		{
			sq->SetBusyNear(1);
		}
        
        return returnValue;
	}

	//Подробно в объявлении Game.h
	int ClearCell(Game::Square *sq_for_cell, Game::Square *sq_color,
				  const FPoint &center_exploid, bool can_clear_stone, float pause_color,
				  bool it_is_bonus, ColorMask killColors, int score, bool leave_chip, bool it_is_boost, bool* is_smthng_destroyed)
	{
		Assert( (sq_color == sq_for_cell) || (sq_color == NULL) || (sq_for_cell == NULL));
        
        bool isSomethingDestroyed = false;
        
		if(sq_for_cell && !sq_for_cell->IsIce() && !it_is_bonus)
		{
			//Сразу нужно убить землю в соседних клетках (внутри стоит таймер запрещающий одновременно убивать все уровни земли)
            
			if (ClearNear( GameSettings::gamefield[sq_for_cell->address + Game::FieldAddress::LEFT ], pause_color))
            {
                isSomethingDestroyed = true;
            }
            
			if (ClearNear( GameSettings::gamefield[sq_for_cell->address + Game::FieldAddress::UP   ], pause_color))
            {
                isSomethingDestroyed = true;
            }
            
			if (ClearNear( GameSettings::gamefield[sq_for_cell->address + Game::FieldAddress::RIGHT], pause_color))
            {
                isSomethingDestroyed = true;
            }
            
			if (ClearNear( GameSettings::gamefield[sq_for_cell->address + Game::FieldAddress::DOWN ], pause_color))
            {
                isSomethingDestroyed = true;
            }
		}
        
		if(sq_for_cell){
			sq_for_cell->GetChip().Deselect();
		}
        
		// Пробуем убрать то что на месте фишки
		bool chip_is_kill = false;
		bool kill_wall = true; // если будет лакрица или мусор, то убиваем только фишку и не трогаем землю под ней
		if(Game::isVisible(sq_color) && Game::activeRect.Contains(sq_color->address.ToPoint()) && !sq_for_cell->IsStone()
           && !sq_for_cell->GetWood() && !sq_for_cell->IsEnergyWood() && (!sq_color->IsBusyCell() || it_is_boost)&& !sq_color->IsIce()
           && !Gadgets::lockBarriers.isLocked(sq_for_cell->address) && !leave_chip)
		{
			int chipColor = sq_color->GetChip().GetColor();
			kill_wall = !sq_color->GetChip().IsLicorice() && !sq_color->GetChip().IsThief() &&!sq_color->GetChip().IsMusor() && (sq_color->GetChip().GetLock() == 0);
			chip_is_kill = sq_color->GetChip().KillChip(center_exploid, sq_for_cell->address, it_is_bonus, false, pause_color);
			sq_color->MarkToBeDestroyed(false);
			if(chip_is_kill)
			{
				Color color = GameSettings::chip_settings[chipColor].color_score;
				GameField::Get()->AddScore(sq_color->address, score, 0.0f, 1.0f, color);
			}
		}
        
		if(Game::isVisible(sq_for_cell) && Game::activeRect.Contains(sq_for_cell->address.ToPoint()) && (!sq_for_cell->IsBusyCell() || it_is_boost)
           && kill_wall && !Gadgets::lockBarriers.isLocked(sq_for_cell->address) && !Gadgets::receivers.IsReceiverCell(sq_for_cell->address))
		{
            if (sq_for_cell->GetWall() > 0)
            {
                isSomethingDestroyed = true;
            }
            if(sq_for_cell->KillSquareFull(can_clear_stone, it_is_bonus, pause_color, chip_is_kill, killColors))
			{
				if (!it_is_boost) {
					sq_for_cell->SetBusyCell(1);
				}
			}
			sq_for_cell->MarkToBeDestroyed(false);
		}
        
        if (is_smthng_destroyed)
        {
            *is_smthng_destroyed = isSomethingDestroyed;
        }
		return 0;
	}

	void PauseControllers(const std::string& controllerName) {
		Core::controllerKernel.PauseControllers(MakeControllerName(controllerName));
	}

	void ContinueControllers(const std::string& controllerName) {
		Core::controllerKernel.ContinueControllers(MakeControllerName(controllerName));
	}

	void KillControllers(const std::string& controllerName) {
		if(controllerName.empty())
		{
			GameField *gameField = GameField::Get();
			for (GameFieldControllerList::iterator it = gameField->_controllers.begin(); it != gameField->_controllers.end(); )
			{
				(*it)->finished = true;
				it = gameField->_controllers.erase(it);
			}
		}else{
			Core::controllerKernel.KillControllers(MakeControllerName(controllerName));
		}
	}


	FPoint GetCenterPosition(const IPoint& pos) {
		int x = (2 * pos.x + 1) * GameSettings::SQUARE_SIDE / 2;
		int y = (2 * pos.y + 1) * GameSettings::SQUARE_SIDE / 2;
		return IPoint(x, y);
	}


	IPoint GetCenterPosition(const FieldAddress& address) {
		int x = (2 * address.GetCol() + 1) * GameSettings::SQUARE_SIDE / 2;
		int y = (2 * address.GetRow() + 1) * GameSettings::SQUARE_SIDE / 2;
		return IPoint(x, y);
	}

	IRect GetCellRect(const FieldAddress& address) {
		return GameSettings::SQUARE_SIDE * IRect(address.ToPoint(), 1, 1);
	}

	bool CheckContainInRadius(float r, const IPoint &a1, const IPoint &a2, Square *sq)
	{
		if(r <= 0 || !sq)
		{
			return false;
		}
		float distance = FPoint(a1).GetDistanceTo(a2);
		float r_big = r*GameSettings::SQUARE_SIDE + 5.f;
		float distance2 = FPoint(a1).GetDistanceTo(a2*GameSettings::SQUARE_SIDE + sq->GetChip().GetPos().Rounded());
		return (distance <= r || distance2 <= r_big);
	}

	Game::Square* GetValidSquare(const Game::FieldAddress &address)
	{
		Game::Square* cell = GameSettings::gamefield[address];
		if (Game::isBuffer(cell))
		{
			GameSettings::squares.push_back(new Game::Square(address));
			GameField::Get()-> UpdateGameField();

			// Нужно переcчитать границы
			GameField::Get() -> _levelScopeUpdated = false;
			GameField::Get() -> _levelScopeUpdateTime = 0.0f;

			cell = GameSettings::squares.back();
		}
		return cell;
	}

	Game::Square* GetValidSquare(int col, int row)
	{
		return GetValidSquare(Game::FieldAddress(col, row));
	}

	bool CanHangBonusOnSquare(Game::Square *sq, bool target)
	{
		return Game::isVisible(sq) && !sq->IsFake() && !sq->IsHardStand()
		       && !sq->IsIce() && sq->HangBonusAllowed() && sq->GetChip().IsSimpleChip()
			   && !sq->GetChip().HasHang() && !sq->ToBeDestroyed() && (target == sq->IsHangBuzy());
	}

	void GetAddressesForHangBonus(std::vector<Game::FieldAddress> &chips)
	{
		bool preferChains = Gadgets::levelSettings.getString("HangBonusOnChains") == "true";

		chips.clear();

		std::vector<Game::FieldAddress> ch1, ch2;

		for (int x = Game::activeRect.LeftBottom().x; x <= Game::activeRect.RightTop().x; x++)
		{
			for (int y = Game::activeRect.LeftBottom().y; y <= Game::activeRect.RightTop().y; y++)
			{
				Game::Square *sq = GameSettings::gamefield[x+1][y+1];
				if(CanHangBonusOnSquare(sq, false)) // && !Game::Spirit::BonusIsFlyingToCell(sq->address))
				{
					Game::FieldAddress fa(x,y);
					if( CheckMaxSeq(fa, 3) )
						ch1.push_back(fa);
					else
						ch2.push_back(fa);
				}
			}
		}

		if( preferChains )
		{
			std::random_shuffle(ch1.begin(), ch1.end());
			std::random_shuffle(ch2.begin(), ch2.end());
			chips.insert(chips.end(), ch1.begin(), ch1.end());
			chips.insert(chips.end(), ch2.begin(), ch2.end());
		}
		else
		{
			chips.insert(chips.end(), ch1.begin(), ch1.end());
			chips.insert(chips.end(), ch2.begin(), ch2.end());
			std::random_shuffle(chips.begin(), chips.end());
		}
	}

	bool DepthFind(const Game::FieldAddress& start_index, int count, int color)
	{
		Game::Square *sq = GameSettings::gamefield[start_index];
		if(!Game::isVisible(sq))
		{
			return false;
		}
		if(!Game::activeRect.Contains(start_index.ToPoint()))
		{
			return false;
		}
		if(sq->IsChecked())
		{
			return false;
		}
		sq->SetChecked(true);//Залачиваем чтобы не обойти 2 раза
		if(!sq->GetChip().IsColor(color))
		{
			return false;
		}
		if(count == 1)
		{
			return true;
		}
		for(size_t i = 0; i < Gadgets::checkDirsChains.count; i++)
		{
			Game::FieldAddress next_pos = start_index + Game::FieldAddress(Gadgets::checkDirsChains[i]);
			if(DepthFind(next_pos, count-1, color))
			{
				return true;
			}
		}
		sq->SetChecked(false); //На выходе разлачиваем
		return false;
	}

	bool CheckMaxSeq(const Game::FieldAddress& start_index, const int &count)
	{
		Game::Square::CURRENT_CHECK++; //Следующая итерация проверок
		Game::Square *sq = GameSettings::gamefield[start_index];
		//Проверяем обходом в глубину
		return DepthFind(start_index, count, sq->GetChip().GetColor());
	}

	std::vector<Game::Square*> ChooseSquaresForBonus(const std::string& bonusChip, size_t count, const AddressVector &seq)
	{
		Assert(seq.size() >= 2);

		const Game::FieldAddress last0 = seq[seq.size() - 2];
		const Game::FieldAddress last1 = seq[seq.size() - 1];

		std::vector<Game::Square*> result;

		// цепляем бонус на фишку, находящуюся над последней взорванной фишкой в цепочке
		if(bonusChip == "up chip") 
		{
			// ищем квадрат куда упадет искомая фишка, и цепляем бонус на него
			Game::FieldAddress fa = last1.Up();
			Game::Square *sq = GameSettings::gamefield[fa];
			if( Game::isVisible(sq) && !sq->IsFake() && sq->IsEmptyForChip() && sq->HangBonusAllowed())
				result.push_back(sq);
			else
				result.push_back( GameSettings::gamefield[last1] );
		}
		// цепляем бонус на фишку, следующую по ходу движения цепочки
		else if(bonusChip == "next")
		{
			Game::FieldAddress fa( 2 * last1.GetCol() - last0.GetCol(), 2 * last1.GetRow() - last0.GetRow());
			Game::Square *sq = GameSettings::gamefield[fa];
			if( Game::isVisible(sq) && !sq->IsFake() && sq->IsEmptyForChip() && sq->HangBonusAllowed())
				result.push_back(sq);
			else
				result.push_back( GameSettings::gamefield[last1] );
		}
		// цепляем бонус на рандомную фишку на экране
		else if(bonusChip == "screen")
		{
			std::vector<Game::FieldAddress> chips;
			Game::GetAddressesForHangBonus(chips);
			for(size_t i = 0; (i < count) && (i < chips.size()); ++i)
			{
				result.push_back( GameSettings::gamefield[chips[i]] );
			}
		}
		// цепляем бонус на последнюю фишку в собранной цепочке (если эта фишка не уничтожается), либо на фишку которая окажется в этой клетке
		else // bonusChip == "chip", "square"
		{
			Game::Square* sq = GameSettings::gamefield[last1];
			if(sq->HangBonusAllowed())
				result.push_back(sq);
		}

		return result;
	}

	// Коэффициенты:
	//   k - замедление движения заднего фона отноcительно поля;
	//   s - коэффициент уменьшения текcтуры фона;
	//   w - размер квадрата текcтуры _fon, в пикcелях.

	void Background_GetUVs(float k, float s, float w, float& u0, float& u1, float& v0, float& v1)
	{
		// Размеры видимой облаcти c учетом текущего маcштабирования
		int w_vis = GameSettings::FIELD_SCREEN_CONST.width;
		int h_vis = GameSettings::FIELD_SCREEN_CONST.height;

		u0 = (GameSettings::fieldX * k        );
		u1 = (GameSettings::fieldX * k + w_vis);
		v0 = (GameSettings::fieldY * k        );
		v1 = (GameSettings::fieldY * k + h_vis);

		float m = w * s;

		u0 /= m;
		u1 /= m;
		v0 /= m;
		v1 /= m;
	}

	ParticleEffectPtr AddEffect(EffectsContainer &cont, const std::string &name)
	{
		ParticleEffect* eff = cont.AddEffect(name);
		if(GameSettings::NEED_SCALE)
		{
			eff->SetScale(GameSettings::SQUARE_SCALE);
		}
		return ParticleEffectPtr(eff);
	}

	void MatrixSquareScale()
	{
		if(GameSettings::NEED_SCALE)
		{
			Render::device.MatrixScale(GameSettings::SQUARE_SCALE);
		}
	}

	void InitFlash(rapidxml::xml_node<> *root_xml)
	{
		ANIM_RESOURCES.clear();
		rapidxml::xml_node<> *node_xml = root_xml->first_node("Flash")->first_node("anim");
		while(node_xml)
		{
			AnimRes res;
			std::string name = Xml::GetStringAttribute(node_xml, "name");
			res.source = Xml::GetStringAttribute(node_xml, "source");
			res.inner_offset = node_xml;
			if(node_xml->first_node("after_pos"))
			{
				res.inner_offset_after = node_xml->first_node("after_pos");
			}else{
				res.inner_offset_after = res.inner_offset;
			}
			res.mirror = Xml::GetBoolAttributeOrDef(node_xml, "mirror", false);
			res.reverse = Xml::GetBoolAttributeOrDef(node_xml, "reverse", false);
			res.can_turn = Xml::GetBoolAttributeOrDef(node_xml, "can_turn", false);
			res.lib = Xml::GetStringAttribute(node_xml, "lib");
			ANIM_RESOURCES[name] = res;
			node_xml = node_xml->next_sibling("anim");
		}
	}
} // namespace Game

