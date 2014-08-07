
#include "stdafx.h"
#include "GameField.h"
#include "GameInfo.h"
#include "GameColor.h"
#include "Game.h"
#include "GameFieldControllers.h"
#include "Match3Spirit.h"

#include "FieldStyles.h"

#include "MyApplication.h"

#include "EditorUtils.h"
#include "ActCounter.h"
#include "Match3Loot.h"
#include "GameFillBonus.h"
#include "Match3Gadgets.h"
#include "Match3Border.h"
#include "WallDrawer.h"
#include "SomeOperators.h"
#include "Match3.h"
#include "LockBarriers.h"
#include "SelectingChip.h"
#include "GameLightningController.h"
#include "GameColor.h"
#include "GameBonuses.h"
#include "ShaderMaterial.h"
#include "Tutorial.h"
#include "LevelInfoManager.h"
#include "EffectWay.h"
#include "PlayerStatistic.h"
#include "LevelEndEffects.h"
#include "DetectBorder2D.h"
#include "BoostKinds.h"
#include "Energy.h"
#include "GameBonuses.h"
#include "FieldBears.h"
#include "SnapGadgetsClass.h"
#include "CellWalls.h"
#include "EnergyReceivers.h"
#include "ChangeEnergySpeedClass.h"
#include "BombField.h"
#include "RyushkiGadget.h"
#include "SquareNewInfo.h"
#include "PictureGenerator.h"
#include "RoomGates.h"
#include "IslandArrows.h"
#include "ReceiverEffects.h"

GameField *GameField::gameField = NULL;

IPoint ddd2[4] = {IPoint(1,0), IPoint(0, 1), IPoint(-1, 0), IPoint(0, -1)};

FPoint oct_vertexs[8];

float romb_square_scale;

bool GameField::VALID = false;

GameField::GameField()
	: _localTime(0.f)
	, _mouseDown(false)
	, _sequenceStarted(false)
	, _needFon(true)
	, _energyPaused(false)
	, _energyResumeDelay(0.0f)
	, _energyTimeScale(1.0f)
	, _energyDistanceTimeScale(1.0f)
	, _timeFromLastMove(0.f)
	, _paused(false)
	, _noMovesTime(0.f)
	, _timerScale(1.f)
	, _levelScopeUpdateTime(0.f)
	, _levelScopeUpdated(true)
	, _lastTooltipMousePos(-1, -1)
	, _wavesShader(NULL)
	, _LCEffectShader(NULL)
	, _screenCopy1Holder(Render::device.Width(), Render::device.Height(), true, true, true)
	, _screenCopy2Holder(Render::device.Width(), Render::device.Height(), true, true, true)
	, _lcEffectTarget(Render::device.Width() / 2, Render::device.Height() / 2, true, false, false)
	, _energyEffectTarget(Render::device.Width() / 2, Render::device.Height() / 2, true, false, false)
	, _screenCopy1(NULL)
	, _screenCopy2(NULL)
	, _restoredGame(false)
	, _fieldMovingTimer(0.f)
	, _seqFires(NULL)
	, _fieldFilling(false)
	, _energyTrakId(-1)
	, _endMoveDelay(0.0f)
	, _energyShaderMaterial(new ShaderMaterial())
	, _energyWaveDelay(0.0f)
	, _energyWaveTimer(5.0f)
	, _orderText(Core::resourceManager.Get<Render::Text>("OrderTooltipText"))
	, _introController(NULL)
	, _resourcePreloaded(false)
	, _needUpdateEnergyEffectTarget(false)
	, _fuuuTester()
	, _moveWaitTimer(0.f)
	, _realFirstChipTime(-10.f)
	, _movesToFinish("")
	, _timerHideAffectedZone(0.f)
	, _affectedZonePulsation(false)
	, _affectedZonePulsationTimer(0.f)
	, _affecredZonePulseTime(2.f)
	, _debug_scale(1.f)
	, _levelObjective(LevelObjective::RECEIVERS)
{
	_moveWaitFull = gameInfo.getConstFloat("Match3_wait_for_start_seq", 0.3f);
	Gadgets::FreeFrontInit();

	ReleaseGameResources();

	_screenCopy1 = &_screenCopy1Holder;
	_screenCopy2 = &_screenCopy2Holder;

	EditorUtils::editor = false;

	Gadgets::Init(this, GameLua::isWindows());

	Game::sandSquareTexture = Core::resourceManager.Get<Render::Texture>("SandPiece");

	Game::InitHangBonusTextures();

	SelectingChips::Init();

	LoadDescriptions();

	initPostEffects();

	//0.666- почти равносторонний вомьиугольник
	//0.5 - вписанный ромб
	romb_square_scale = ( 1 - gameInfo.getConstFloat("SQUARE_CONER_ZONE", 0.333f))*GameSettings::SQUARE_SIDEF;
	
	const float oct_a = GameSettings::SQUARE_SIDEF/2.f;
	const float oct_b = romb_square_scale - oct_a;

	oct_vertexs[0] = FPoint(oct_a,oct_b);
	oct_vertexs[1] = FPoint(oct_b, oct_a);
	oct_vertexs[2] = FPoint(-oct_b, oct_a);
	oct_vertexs[3] = FPoint(-oct_a, oct_b);
	oct_vertexs[4] = FPoint(-oct_a, -oct_b);
	oct_vertexs[5] = FPoint(-oct_b, -oct_a);
	oct_vertexs[6] = FPoint(oct_b, -oct_a);
	oct_vertexs[7] = FPoint(oct_a, -oct_b);
	GameField::VALID = true;

	_globalHang.MakeLittleBomb();

	boostList.StandardInit();   //заполняем стандартные бусты
}

FPoint draw_dirs[8] = {FPoint(1.f, 0.5f), FPoint(1.f, 1.f), FPoint(0.5f, 1.f), FPoint(0.f, 1.f), FPoint(0.f,0.5f), FPoint(0.f,0.f), FPoint(0.5f,0.f), FPoint(1.f,0.f)};

void GameField::LoadDescriptions()
{
	gameInfo.ReloadConstants(); //Перезагружаем константы игры

	Xml::RapidXmlDocument doc_desc("GameDescriptions.xml");
	rapidxml::xml_node<> *xml_root = doc_desc.first_node();

	//Тултипы
	rapidxml::xml_node<> *xml_field = xml_root->first_node("Field");
	_tooltip.Init(xml_field->first_node("Tooltip"));
	_orderTooltip.Init(xml_field->first_node("OrderTooltip"));

	Gadgets::levelSettingsDefault.Load(xml_root->first_node("level_setings_default"));
	Gadgets::levelSettings = Gadgets::levelSettingsDefault;

	GameSettings::score.Load(xml_root->first_node("ScoreSettings"));

	Gadgets::wallDrawer.Init(xml_root);
	DetectBorder2D::InitGame(xml_root);

	levelBorder.Init("level");

	Game::MUSOR_PIECES_TEX = Core::resourceManager.Get<Render::Texture>("MusorPieces");

	Energy::Settings::Load( xml_root->first_node("Energy") );

	_energyLightTimer = -1.f;

	FieldStyles::Load(xml_root->first_node("FieldStyles"));

	GameSettings::chip_settings.clear();
	rapidxml::xml_node<> *xml_chip_colors = xml_root->first_node("ChipColors");
	if(xml_chip_colors)
	{
		rapidxml::xml_node<> *xml_chip_color = xml_chip_colors->first_node("color");
		while(xml_chip_color)
		{
			int n = Xml::GetIntAttribute(xml_chip_color, "n");
			GameSettings::chip_settings[n].Load(xml_chip_color);
			xml_chip_color = xml_chip_color->next_sibling("color");
		}
	}
	Game::ChipColor::InitGame(xml_root);
	Game::ArrowBonus::InitGame(xml_root);
	Game::InitFlash(xml_root);

	Game::ChipColor::YOffset_Chip = gameInfo.getConstFloat("YOffset_Chip")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_ChipBack = gameInfo.getConstFloat("YOffset_ChipBack")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_ChipBackIce = gameInfo.getConstFloat("YOffset_ChipBackIce")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_Choc = gameInfo.getConstFloat("YOffset_Choc")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_ChocFabric = gameInfo.getConstFloat("YOffset_ChocFabric")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_WOOD[0] = gameInfo.getConstFloat("YOffset_WOOD_0")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_WOOD[1] = gameInfo.getConstFloat("YOffset_WOOD_1")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_WOOD[2] = gameInfo.getConstFloat("YOffset_WOOD_2")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_WOOD[3] = gameInfo.getConstFloat("YOffset_WOOD_3")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::YOffset_WOOD[4] = gameInfo.getConstFloat("YOffset_WOOD_4")*GameSettings::SQUARE_SCALE;

	Game::ChipColor::YOffset_STONE = gameInfo.getConstFloat("YOffset_STONE")*GameSettings::SQUARE_SCALE;
	Game::ChipColor::CHIP_REMOVE_DELAY = gameInfo.getConstFloat("CHIP_REMOVE_DELAY");

	LockBarrierBase::YOffset_LOCK = gameInfo.getConstFloat("YOffset_LOCK")*GameSettings::SQUARE_SCALE;
	GameSettings::SEQ_FAST_MOVE_TIME = gameInfo.getConstFloat("SEQ_FAST_MOVE_TIME");

	Game::ChipColor::CHIP_START_HIDE = gameInfo.getConstFloat("CHIP_START_HIDE");
	Game::Square::SQUARE_START_EFFECT_WALL = gameInfo.getConstFloat("SQUARE_START_EFFECT_WALL");
	Game::Square::SQUARE_HIDE_WALL = gameInfo.getConstFloat("SQUARE_HIDE_WALL");
	Game::Square::WALL_EFFECT_TO_FRONT_PAUSE = gameInfo.getConstFloat("WALL_EFFECT_TO_FRONT_PAUSE");
	//Скорость перевода на передний план эффекта разрушения земли
	Game::Square::WALL_EFFECT_TO_FRONT_TIME = gameInfo.getConstFloat("WALL_EFFECT_TO_FRONT_TIME");
	Game::ChipColor::CHIP_TAP_OFFSET = IPoint(0, int(gameInfo.getConstInt("YOffset_ChipTap")*GameSettings::SQUARE_SCALE));

	SelectingChips::Init();

	SnapGadgetReleaseBorderElement::COLOR_LIGHT = gameInfo.getConstString("ENERGY_LEVEL_WALL_COLOR", "#0000ff80");
	Utils2D::PictureCellGenerator::LoadSettings();
	Island::InitGame();
	Game::Square::InitGame();
}

bool is_energy_full0(Game::Square *sq)
{
	return Game::isVisible(sq) && Energy::field.FullOfEnergy(sq->address);
}

bool is_energy_visible(Game::Square *sq)
{
	return Game::isVisible(sq) && Energy::field.EnergyExists(sq->address);
}

void GameField::UpdateEnergyEffectContainer(float dt)
{
	_energyShaderEffectContainer.Update(dt);
	_energyShaderEffectContainer.Finish();

	if(_energyWaveDelay > 0.0f)
		_energyWaveDelay -= dt;

	if(_energyWaveTimer > 0.0f)
		_energyWaveTimer -= dt;

	if(_energyWaveTimer <= 0.0f && _energyWaveDelay <= 0.0f)
	{
		FPoint pos = math::random( FPoint(0,0), GameSettings::VIEW_RECT.RightTop() );
		AddEnergyWave(GameSettings::ToFieldPos(pos), "EnergyWave");
	}
}

void GameField::AddEnergyWave(FPoint pos, const std::string &effName)
{
	ParticleEffectPtr  effect = Game::AddEffect(_energyShaderEffectContainer, effName);
	effect->SetPos(pos);
	effect->Reset();
	_needUpdateEnergyEffectTarget = true;

	_energyWaveDelay = 0.01f;
	_energyWaveTimer = math::random(1.0f, 4.0f);
}

void GameField::AddEnergyArrowWave(FPoint pos, BYTE dir)
{
	std::string effName;
	if( dir == (Game::Hang::ARROW_R | Game::Hang::ARROW_L) )
		effName = "EnergyWaveHorizontal";
	else if( dir == (Game::Hang::ARROW_U | Game::Hang::ARROW_D) )
		effName = "EnergyWaveVertical";
	else if( dir == Game::Hang::ARROW_4 )
		effName = "EnergyWave4x";

	if( !effName.empty() )
	{
		ParticleEffectPtr effect = Game::AddEffect( _energyShaderEffectContainer, effName);
		effect->SetPos(pos);
		effect->Reset();
		_needUpdateEnergyEffectTarget = true;
	}
}

GameField* GameField::Get() 
{
	MyAssert(gameField != NULL);
	return gameField;
}

void GameField::ComputeSquareDistRec()
{
	if (Gadgets::receivers.TotalCount() == 0)
	{
		return;
	}

	IPoint pos = Gadgets::receivers.MoreFar()->GetIndex();

	int x = pos.x;
	int y = pos.y;

	const int dx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
	const int dy[8] = {0, 0, 1, -1, 1, -1, 1, -1};

	float k = 8.f;

	int beg = 0;
	int end = 0;

	for (int i = 0; i< GameSettings::fieldWidth; i++)
	{
		for (int j = 0; j< GameSettings::fieldHeight; j++)
		{
			if(pos == IPoint(i, j))
			{
				Gadgets::squareDistRec[i][j] = k + abs(i-x-1) + abs(j-y-1);

				_queue[end] = IPoint(i, j);
				end++;
			} else {
				Gadgets::squareDistRec[i][j] = -1.f;
			}
		}
	}

	while (beg < end)
	{
		int e = end;
		for (int i = beg; i<e; i++)
		{
			float d = Gadgets::squareDistRec[_queue[i].x][_queue[i].y];
			for (int q = 0; q<8; q++)
			{
				IPoint pos = IPoint(_queue[i].x + dx[q], _queue[i].y + dy[q]);
				if (pos.x >= 0 && pos.x < GameSettings::fieldWidth && pos.y >= 0 && pos.y < GameSettings::fieldHeight) //В рамках поля
				{
					if (GameSettings::recfield[pos.x+1][pos.y+1])//По клетке может течь энергия
					{
						float d1 = d + sqrt(abs(dx[q])+abs(dy[q])+0.f);

						if (Gadgets::squareDistRec[pos.x][pos.y]<0)
						{
							//В клетке еще небыли
							Gadgets::squareDistRec[pos.x][pos.y] = d1;
							_queue[end] = pos;
							end++;
						} else {
							//Если были в клетке, то проверим минимальное ли в ней значение расстояния до ресивера
							if (Gadgets::squareDistRec[pos.x][pos.y] > d1)
							{
								Gadgets::squareDistRec[pos.x][pos.y] = d1;
							}
						}
					}
				}
			}
			//Проверяем порталы
			std::vector<Game::Square*> vec;
			size_t  count = GameSettings::GetOtherPortalsSquares(Game::FieldAddress(_queue[i].x, _queue[i].y), vec);
			for(size_t i = 0; i < count; i++)
			{
				IPoint pos = vec[i]->address.ToPoint() - IPoint(1,1);
				if (GameSettings::recfield[pos.x+1][pos.y+1])
				{
					float d1 = d + 1.f;
					if (Gadgets::squareDistRec[pos.x][pos.y]<0)
					{
						Gadgets::squareDistRec[pos.x][pos.y] = d1;

						_queue[end] = pos;
						end++;
					} else {
						if (Gadgets::squareDistRec[pos.x][pos.y] > d1)
						{
							Gadgets::squareDistRec[pos.x][pos.y] = d1;
						}
					}
				}
			}
		}
		beg = e;
	}
}



void GameField::ComputeSquareDist()
{
	//if (Gadgets::receivers.TotalCount() == 0)
	//{
	//	return;
	//}

	int beg = 0;
	int end = 0;
	float k = 400000.f;		//В источнике эрергии - максимум
	float min_distance = k;

	for (int i = 0; i< GameSettings::fieldWidth; i++)
	{
		for (int j = 0; j< GameSettings::fieldHeight; j++)
		{
			if(Gadgets::square_new_info.IsEnergySourceSquare(IPoint(i, j)))
			{
				Gadgets::squareDist[i][j] = k;
				_queue[end] = IPoint(i, j);
				end++;
			} else {
				Gadgets::squareDist[i][j] = -1.f;
			}
		}
	}

	while (beg < end)
	{
		int e = end;
		for (int i = beg; i<e; i++)
		{
			float d = Gadgets::squareDist[_queue[i].x][_queue[i].y];
			for (size_t q = 0; q < Gadgets::checkDirsInfo.count; q++)
			{
				IPoint delta = Gadgets::checkDirsInfo[q];
				IPoint pos = _queue[i] + delta;
				if (pos.x >= 0 && pos.x < GameSettings::fieldWidth && pos.y >= 0 && pos.y < GameSettings::fieldHeight)
				{
					if (Game::isVisible(GameSettings::gamefield[pos.x + 1][pos.y + 1]))
					{
						float d1 = d - sqrt(abs(delta.x)+abs(delta.y*4.f/3)+0.f);

						if (Gadgets::squareDist[pos.x][pos.y] < 0)
						{
							Gadgets::squareDist[pos.x][pos.y] = d1;
							min_distance = math::min(min_distance, d1);
							_queue[end] = pos;
							end++;
						} else {
							if (Gadgets::squareDist[pos.x][pos.y] < d1)
							{
								Gadgets::squareDist[pos.x][pos.y] = d1;
								min_distance = math::min(min_distance, d1);
							}
						}
					}
				}
			}
			//Проверяем порталы
			std::vector<Game::Square*> vec;
			size_t  count = GameSettings::GetOtherPortalsSquares(Game::FieldAddress(_queue[i].x, _queue[i].y), vec);
			for(size_t i = 0; i < count; i++)
			{
				float d1 = d - 1.f;
				IPoint pos = vec[i]->address.ToPoint(); // - IPoint(1,1);
				if (Gadgets::squareDist[pos.x][pos.y] < 0)
				{
					Gadgets::squareDist[pos.x][pos.y] = d1;
					min_distance = math::min(min_distance, d1);
					_queue[end] = pos;
					end++;
				} else {
					if (Gadgets::squareDist[pos.x][pos.y] < d1)
					{
						Gadgets::squareDist[pos.x][pos.y] = d1;
						min_distance = math::min(min_distance, d1);
					}
				}
			}
		}
		beg = e;
	}

	for (int i = 0; i< GameSettings::fieldWidth; i++)
	{
		for (int j = 0; j< GameSettings::fieldHeight; j++)
		{
			if(Gadgets::squareDist[i][j] > 0)
			{
				Gadgets::squareDist[i][j] -= min_distance;
				Assert(Gadgets::squareDist[i][j] >= 0);
			}
		}
	}

	ComputeSquareDistRec();
}

void GameField::LoadLevelForEdit(const std::string &name)
{
	KillAllEffects();

	EditorUtils::lastLoadedLevel = name;

	// Разыcкиваем уровень в cпиcке...
	int index = levelsInfo.GetLevelIndex(name);
	if (index != -1){
		gameInfo.setLocalInt("current_level", index);
		_level = index;
	}

	Tutorial::luaTutorial.End();

	// Загружаем уровень из файла
	if (!LoadLevelRapid(name)) 
	{
		std::string  text = "Невозможно найти уровень " + name + "!";
		Assert2(false, text);
		return;
	}	
	ComputeFieldRect();

	_paused = false;
}

void GameField::LoadLevelAndRun(const std::string &name)
{
	if (!LoadLevelRapid(name)) 
	{
        Assert(false);
		std::string text = "Невозможно найти уровень " + name + "!";
		return;
	}

	InitLevelAfterLoad(false);

	if (gameInfo.getConstBool("CollectStatistics"))
	{
		playerStatistic.StartCollect("LevelStat.csv", "LevelName;Date;Time;Result;TotalMoves;SpentMoves;TotalReceivers;ActiveReceivers;AvgPossibleMoves;NeededMovesWhenLose");
		playerStatistic.SetValue("LevelName", name);

		SYSTEMTIME stm;
		utils::GetLocalTime(stm);

		std::string dateStr = utils::lexical_cast(stm.wDay) + "." + utils::lexical_cast(stm.wMonth) + "." +utils::lexical_cast(stm.wYear);
		playerStatistic.SetValue("Date", dateStr);

		std::string timeStr = utils::lexical_cast(stm.wHour) + ":" + utils::lexical_cast(stm.wMinute);
		playerStatistic.SetValue("Time", timeStr);

		int moves = Match3GUI::ActCounter::GetCounter();
		playerStatistic.SetValue("TotalMoves", utils::lexical_cast(moves));
		playerStatistic.SetValue("SpentMoves", "0");

		playerStatistic.SetValue("TotalReceivers", utils::lexical_cast(Gadgets::receivers.TotalCount()));

		Game::totalPossibleMoves = 0;
	}
}

void GameField::ApplyFieldStyle()
{
	_backgroundEmpty = Core::resourceManager.Get<Render::Texture>(FieldStyles::current->bg_empty);
	
	Gadgets::wallDrawer.LoadLevel();

	FieldStyles::current->Upload();

	//std::string fonName = "Fon_" + FieldStyles::current->name;
	std::string scopeName = "LevelScope_" + FieldStyles::current->name;
	//_fon = Core::resourceManager.Get<Render::Texture>(fonName);
	
	Core::resourceManager.Reload<Render::Texture>(scopeName, ResourceLoadMode::Sync);

	if(FieldStyles::current->parallax.isUsing && FieldStyles::current->name == "MagickForest"){
		FieldStyles::ParallaxScene& base = FieldStyles::current->parallax.parallaxBackground.base;
		
		Core::resourceManager.Reload<Render::Texture>(base.texture->textureID, ResourceLoadMode::Sync);

		base.texture -> setFilteringType(Render::Texture::BILINEAR);
		base.texture -> setAddressType(Render::Texture::REPEAT);

		std::vector<FieldStyles::ParallaxScene>::iterator it = FieldStyles::current->parallax.parallaxBackground.backgrounds.begin();
		std::vector<FieldStyles::ParallaxScene>::iterator end = FieldStyles::current->parallax.parallaxBackground.backgrounds.end();
		
		for(; it != end; ++it) {
			it->texture -> setFilteringType(Render::Texture::BILINEAR);
			it->texture -> setAddressType(Render::Texture::REPEAT);

			Core::resourceManager.Reload<Render::Texture>(it->texture->textureID, ResourceLoadMode::Sync);
		}
	}else{
		//Core::resourceManager.Reload<Render::Texture>(fonName, ResourceLoadMode::Sync);

		//_fon->setFilteringType(Render::Texture::BILINEAR);
		//_fon->setAddressType(Render::Texture::REPEAT);
	}
	
	_backgroundEmpty->setFilteringType(Render::Texture::BILINEAR);
	_backgroundEmpty->setAddressType(Render::Texture::REPEAT);
	
	levelBorder.LoadLevel();
}

bool GameField::IsPaused() const
{
	return _paused;
}

//смена флажка движения камеры
void GameField::SetFieldMoving(bool value)
{
	if (BoostList::HardPtr currentActiveBoost = _currentActiveBoost.lock())
	{
		currentActiveBoost->OnFieldMoving(value);	//необходимо уведомить активный буст
	}

	_fieldMoving = value;
}

void GameField::FillChameleonVector()
{
	_chameleons.clear();

	for (int col = 0; col < GameSettings::FIELD_MAX_WIDTH; col++) 
	{
		for (int row = 0; row < GameSettings::FIELD_MAX_HEIGHT; row++) 
		{
			Game::FieldAddress fa(col, row);
			Game::Square *sq = GameSettings::gamefield[fa];
			if (Game::isVisible(sq) && sq->GetChip().IsChameleon())
			{
				_chameleons.push_back(fa);
			}
		}
	}
}

void GameField::FindBestChameleonColors() 
{
	for(Game::FieldAddress fa : _chameleons)
	{
		Game::Square *sq = GameSettings::gamefield[fa];

		Assert(Game::isVisible(sq));
		Assert(sq->GetChip().IsChameleon());
		
		int color = GetPreferredChameleonColor(fa);
		sq->GetChip().SetColor(color);
	}
}

int GameField::GetPreferredChameleonColor(const Game::FieldAddress& index) 
{
	Assert(index.IsValid());
	Assert(GameSettings::gamefield[index]->GetChip().IsChameleon());

	int maxColor = -1;
	int maxArea = -1;
	int count_colors = Gadgets::levelColors.GetCount();
	for (int i = 0; i < count_colors; ++i) 
	{
		int color = Gadgets::levelColors[i];
		
		int area = GetConnectedArea(index, color);
		
		if (maxArea < area) 
		{
			maxArea = area;
			maxColor = color;
		}
	}
	Assert(maxArea > 0);
	Assert(maxColor > 0);
	return maxColor;
}

GameField::AddressSetPtr GameField::GetConnectedCells(const Game::FieldAddress& index, int color)
{
	// реализуем поиcк в ширину

	// множеcтво найденных ячеек:
	GameField::AddressSetPtr counted(new GameField::AddressSet()); 

	// "рабочее" множеcтво - здеcь находитcя граница найденных ячеек:
	GameField::AddressSet working; 

	// Пока cпиcок граничных точек не пуcт
	working.insert(index);

	while (working.size() > 0) 
	{
		// помечаем граничные точки как поcчитанные
		// gsh: на некоторых уровнях граничных клеток cлишком много и маccив получается здоровенный
		counted->insert(working.begin(), working.end());

		// Проходим по граничным точкам и пытаемcя их раcширить
		// (cтроим новую границу newWorking)
		GameField::AddressSet newWorking;
		
		for (AddressSet::iterator i = working.begin(); i != working.end(); ++i) 
		{
			const int NEIGHBOURS = 4;
			Game::FieldAddress neighbours[NEIGHBOURS] = {i->Down(), i->Up(), i->Left(), i->Right()};

			for (int i = 0; i < NEIGHBOURS; ++i) 
			{
				Game::FieldAddress& a = neighbours[i];

				// Еcли этот адреc вылезает за пределы поля, он нам не интереcен
				if (!a.IsValid())
				{
					continue;
				}

				Game::Square* s = GameSettings::gamefield[a];
				
				if (s->IsFake())
				{
					continue;
				}

				// Еcли адреc не подходит по цвету, он нам не интереcен
				if (color != s->GetChip().GetColor() && !s->GetChip().IsChameleon()) 
				{
					continue;
				}

				// Еcли этот адреc уже поcчитан, нам он не интереcен
				if (counted->count(a))
				{
					continue;
				}

				// Теперь мы знаем точно, что адреc нам интереcен
				newWorking.insert(a);
			}
		}
		// Забываем cтарую границу
		working = newWorking;
	}
	return counted;
}

int GameField::GetConnectedArea(const Game::FieldAddress& index, int color)
{
	return int(GetConnectedCells(index, color)->size());
}

void GameField::InitLevelAfterLoad(const bool &editor)
{
	levelsInfo.levelStarted = false;
	Match3GUI::LootPanel::SetScore(0);

	_reshuffleChipsFlying = -1;
	_endLevel.InitLevel();

	_tutorialHangBonusSquares.clear();
	_tutorialHangBonusTypes.clear();

	Tutorial::luaTutorial.End();

	StopEnergySourceEffects();

	_energyTimeScale = 1.0f;
	_energyDistanceTimeScale = 1.0f;

	_localTime = 0.0f;

	_endMoveDelay = 0.0f;

	_movingMonsters._needMoveMonsters = 0;

	_startSoundDisable = false;
	PauseEnergy();

	GameSettings::need_inc_wall_squares = 0;
	GameSettings::need_inc_growing_wood = 0;

	_needUpdateSand = false;
	
	_paused = false;
	_isCurrentMoveActive = false;

	_toolInfoTextShow = false;
	_toolAddress = Game::FieldAddress();

	_energyArrowMoving = false;

	_timerScale = 1.f;

	Game::timeFromLastBreakingWallSound = 0.f;

	Game::ChipColor::tutorialChainLength = 0;

	_maybeMoveGadget.NeedUpdate();
	
	_noMovesTime = 0.f;

	_maybeMoveGadget.Clear();

	Game::KillControllers();

	Core::controllerKernel.Update(0.f);
	Game::Spirit::ResetCounters();
	
	std::vector<IPoint> vec;
	Gadgets::square_new_info.EnergySource_Get(vec);
	if(!vec.empty()){
		_activeEnergy = vec.front();
	}else{
		Assert(_levelObjective != LevelObjective::ENERGY);
	}

	Gadgets::freeFrontDetector.LoadLevel();
	Gadgets::receivers.InitLevel();

	Match3::Init();

	_fieldMoving = false;


	if(!EditorUtils::editor)
	{
		//ИнтрО.
		if(_introController)
		{
			Assert(false);
			KillAllControllers();
			_introController = NULL;
		}
		_introController = new StartFieldMover();
		Game::AddController(_introController);
		_timeFromLastMove = 0.2f;		
	}
		

	Game::UpdateVisibleRects();
	Game::UpdateTargetRect();

	_eSourceAccentTime = 0.f;
	_needESourceAccent = true;

	_levelStarted = false;

	for (size_t i = 0; i < GameSettings::squares.size(); i++)
	{
		GameSettings::squares[i]->Init();
	}

	Gadgets::lockBarriers.InitLevel();

	_blocked = 0;

	_sourceOpened = false;

	_mouseDown = false;
	_sequenceStarted = false;
	_chipSeq.clear();
	
	Energy::field.Release();
	if(!editor)
	{
		// перемешиваем головоломку
		FillLevel();
		Energy::field.Init();
		//Gadgets::square_new_info.EnergySource_Start();
		Gadgets::snapGadgets.PrepareLevel();
	}

	Game::ChipColor::CURRENT_IN_SEQUENCE_ITER = 0;
	//ComputeSquareDist();
	//Gadgets::snapGadgets.UpdateActive();

	Game::Square::CURRENT_CHECK = 1;

	_orderTooltip.HideNow();

	Gadgets::wallDrawer.InitAfterLoadLevel();
	UpdateGameField(true);

	_useBoostLittleBomb = false;

	_currentActiveBoost = BoostList::WeakPtr();
	
	_movingMonsters._haveMonsters = false;

	//выставляем флажок наличия монстриков на уровне, чтобы зря не проверять потом
	for (Game::Square *sq : GameSettings::squares)
	{
		if (sq->GetChip().IsThief())
		{
			_movingMonsters._haveMonsters = true;
		}
	}

	if (!Gadgets::movingMonstersSources._sources.empty())
	{
		_movingMonsters._haveMonsters = true;
	}

	_movingMonsters._canWakeMonsters = true;

	Tutorial::luaTutorial.Run();

	if (!editor && Tutorial::luaTutorial.GetScriptName().empty()) {
		// если на уровне есть туториал, то это его обязанность показать приемники, когда нужно
		Gadgets::receivers.AcceptMessage(Message("Start"));
	}

	///// удалить после теста - не требуется /////////////
	_fuuuTester.Clear();
	//////// по сюда ///////
}

GameField::~GameField()
{
	GameField::VALID = false;
	//AcceptMessage(Message("Release"));

	delete _energyShaderMaterial;

	ReleaseGameResources();

	_maybeMoveGadget.Clear();

	Gadgets::receivers.Clear();
	Gadgets::ryushki.ClearRyushki();

	Gadgets::Release();

	AcceptMessage(Message("ClearLevel"));
	Energy::field.Release();
	Place2D::Release();

	KillAllControllers();
}

void GameField::DrawBackground()
{
	if(FieldStyles::current->fon_static_texture)
	{
		Render::device.PushMatrix();
		Render::device.MatrixTranslate( math::Vector3(0.0f, 0.0f, ZBuf::BACKGROUND) );
		Render::device.SetBlend(false);
		FieldStyles::current->fon_static_texture->Draw(0.f, 0.f, Render::device.Width(), Render::device.Height(), FRect(0.f, 1.f, 0.f, 1.f));
		Render::device.SetBlend(true);
		FieldStyles::current->fonDrawer.DrawClouds(1.f, FPoint(GameSettings::fieldX, GameSettings::fieldY));
		Render::device.PopMatrix();
	}
}

void GameField::RenderToTargets()
{
	math::Vector3 offset(-GameSettings::fieldX, -GameSettings::fieldY, 0.0f);

	if(_needUpdateEnergyEffectTarget || _energyEffectTarget.NeedRedraw())
	{
		_energyEffectTarget.BeginRendering(Color(128, 128, 255, 255));
		Render::device.PushMatrix();
		Render::device.MatrixScale(0.5, 0.5, 1.0);
		Render::device.MatrixTranslate(offset);
		_energyShaderEffectContainer.Draw();
		Render::device.PopMatrix();
		_energyEffectTarget.EndRendering();
		
		_needUpdateEnergyEffectTarget = !_energyShaderEffectContainer.IsFinished();
	}
}

bool GameField::hasWavesInScreen() const
{
	return !_waves.empty(); 
}

void  GameField::StartStencilWrite(BYTE val, BYTE mask)
{
	Render::device.SetStencilFunc(Render::StencilFunc::ALWAYS, val, mask);
	Render::device.SetStencilOp(Render::StencilOp::KEEP, Render::StencilOp::KEEP, Render::StencilOp::REPLACE);
}

void  GameField::StartStencilTest(Render::StencilFunc::Type func, BYTE val, BYTE mask)
{
	Render::device.SetStencilFunc(func, val, mask);
	Render::device.SetStencilOp(Render::StencilOp::KEEP, Render::StencilOp::KEEP, Render::StencilOp::KEEP);
}

void GameField::DrawField()
{
	//Подготавливаем массивы летающих клеток
	std::list<Game::Square*> flying_squares_up, flying_squares_down;
	for(auto sq: GameSettings::squares)
	{
		if(sq->IsFlyType(Game::Square::FLY_NO_DRAW) || sq->IsFlyType(Game::Square::FLY_STAY))
		{
			continue;
		}
		if(sq->IsAddToDownDraw())
		{
			flying_squares_down.push_back(sq);
		}else{
			flying_squares_up.push_back(sq);
		}
	}
	flying_squares_down.sort(Game::Square::FlySort);
	flying_squares_up.sort(Game::Square::FlySort);

	Render::device.SetDepthTest(true);
	Render::device.SetDepthWrite(true);
	/************************************************************
	 Отрисовка с включенным Z-тестом и записью в буфер глубины
	************************************************************/
	// Здесь рисуем только полностью непрозрачные объекты, в произвольном
	// порядке (но лучше сортировать от ближних к дальним)

	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(-GameSettings::fieldX, -GameSettings::fieldY, 0.0f));

	Gadgets::wallDrawer.DrawStone();
	Gadgets::wallDrawer.DrawIce();
	Gadgets::wallDrawer.DrawCyclops();

	Gadgets::wallDrawer.DrawWood();
	Gadgets::wallDrawer.DrawGround();
	
	DrawEnergy();

	Gadgets::wallDrawer.DrawFieldBase();

	Render::device.PopMatrix();
	
	Render::device.SetDepthWrite(false);

	/************************************************************
	 Отрисовка с включенным Z-тестом без записи в буфер глубины
	************************************************************/
	
	DrawBackground();

	Render::device.PushMatrix();
	{
		Render::device.MatrixTranslate(math::Vector3(-GameSettings::fieldX, -GameSettings::fieldY, 0.0f));

		Render::device.PushMatrix();
		Render::device.MatrixTranslate(math::Vector3(0.f, 0.f, ZBuf::ISLANDS_BACK));
		for(auto sq : flying_squares_down)
		{
			sq->DrawFly();
		}
		Render::device.PopMatrix();

		Render::device.SetStencilTest(true);
		GameField::StartStencilTest(Render::StencilFunc::EQUAL, 0xC0, 0xF0);
		Energy::field.DrawParticles();
		Render::device.SetStencilTest(false);

		Gadgets::bears.Draw(true);

		Gadgets::receivers.DrawDown(); //Часть ресивера рисуется между подложкой и энергией

		Gadgets::wallDrawer.DrawGroundTransparent();
		Gadgets::wallDrawer.DrawGroundBorders();
		Gadgets::wallDrawer.DrawWoodBorders();
		levelBorder.Draw();

		// отрисовываем те части клеток, которые находятся за стенами и льдом
		Render::device.PushMatrix();
		Render::device.MatrixTranslate( math::Vector3(0.0f, 0.0f, ZBuf::CHIPS) );
		Gadgets::receivers.DrawBase();
		DrawSquares();
		Render::device.PopMatrix();

	
		//Render::device.SetStencilTest(true);
		//GameField::StartStencilTest(Render::StencilFunc::NOTEQUAL, 0xA0, 0xF0); // границы льда не будут рисоваться поверх стен...
		Gadgets::wallDrawer.DrawIceBorders();
		//GameField::StartStencilTest(Render::StencilFunc::NOTEQUAL, 0xB0, 0xF0); // ...а границы стен поверх льда
		Gadgets::wallDrawer.DrawStoneBorders();
		//Render::device.SetStencilTest(false);
		Gadgets::wallDrawer.DrawCyclopsBorders();



		Render::device.SetDepthTest(false);
		/************************************************************
				 Отрисовка без использования буфера глубины
		************************************************************/

		Gadgets::bears.DrawOverlay();

		DrawSequenceAffectedZone(); // подсветка области поражения

		//if(!_chipSeq.empty()) {
		//	// выделение текущей цепочки
		//	_selectionChip.Draw(_chipSeq, _localTime);
		//}
	
		if(EditorUtils::editor){
			DrawEditorChips();
			Gadgets::lockBarriers.Draw();
		}
		_effUnderChipsCont.DrawBlend();

		// риcуем эффекты
		for (int k = 1; k <= 3; k++)
		{
			for(GameFieldController *c : _controllers)
			{
				if (c->z == k)
					c->DrawUnderChips();
			}
		}

		Gadgets::snapGadgets.DrawGame();
		DrawChips(0); // обычные фишки, колышки, монстрики
		Gadgets::gates.Draw();
		Gadgets::receivers.DrawUp();
		Gadgets::lockBarriers.DrawUp();
		DrawChips(1); // фишки с бонусами
		DrawChips(2); // летящие и падающие фишки

		// Все что рисуется выше этого комментария уже должно быть отсортировано и корректно рисоваться
		// с учетом псевдо-3д. Все что ниже как повезет)


		// Выводим рюшки,F раcположенные под полем. Для них: (zLevel <= -1)
		// Маccив рюшек отcортирован!
		Gadgets::ryushki.DrawLowLevel();

		//Рисуем бонусы под землей
		Gadgets::ug_prises.DrawUnderWalls();

		// Риcуем содержимое клеток (под фишками)

		Gadgets::wallDrawer.DrawSand();

		//Color bgColor = FieldStyles::current->bgEmptyColor;
		//for (int x = Game::visibleRect.LeftBottom().x - 1; x <= Game::visibleRect.RightTop().x + 1; x++)
		//{
		//	for (int y = 0; y <= Game::visibleRect.RightTop().y; y++)
		//	{
		//		Game::Square *s = GameSettings::gamefield[x+1][y+1];
		//		if(Game::isVisible(s) && s->GetSandTime() > 0)
		//		{
		//			if(Energy::field.EnergyExists(s->address)) //Рисуем фоном, а фон может быть разный
		//			{
		//				Render::device.SetTexturing(false);
		//				Color col(uint8_t(_energyColor[0]*255), uint8_t(_energyColor[1]*255), uint8_t(_energyColor[2]*255), uint8_t(_energyColor[3]*255));
		//				s->DrawSand(col, IRect(0,0,1,1));
		//				Render::device.SetTexturing(true);
		//			}else{
		//				_backgroundEmpty->Bind();
		//				s->DrawSand(bgColor, _backgroundEmpty->getBitmapRect());
		//			}
		//		}
		//	}
		//}
	
	
		for(auto sq : flying_squares_up)
		{
			sq->DrawFly();
		}

		Island::DrawArrows();


		//_effContUpSquare.Draw();

		// Выводим рюшки, раcположенные над полем. Для них: (zLevel >= 1 && zLevel < 4)
		// Маccив рюшек отcортирован!
		Gadgets::ryushki.DrawAverageLevel();

		Gadgets::cellWalls.Draw();

		Gadgets::bombFields.Draw();

		Gadgets::ryushki.DrawHighLevel();

		//if (gameInfo.IsDevMode() && !EditorUtils::editor && Core::mainInput.IsShiftKeyDown())
		//{
		//	_maybeMoveGadget.DrawDebug();
		//}
		_effCont.DrawBlend();

		Gadgets::snapGadgets.DrawEdit();
		
		//рисуем бонус
	
		if (BoostList::HardPtr currentActiveBoost = _currentActiveBoost.lock())
		{
			currentActiveBoost->Draw();
		}

		// риcуем эффекты
		for (int k = 1; k <= 3; k++)
		{
			for (GameFieldControllerList::iterator ic = _controllers.begin(); ic != _controllers.end(); ++ic)
			{
				if ((*ic)->z == k)
					(*ic)->Draw();
			}
		}

		
		_effTopCont.DrawBlend();

		if(EditorUtils::draw_debug)
		{
			DetectBorder2D::Draw();
			Gadgets::freeFrontDetector.DrawEdit();
		}

		_fuuuTester.Draw();
	}

	Render::device.PopMatrix();
}

void GameField::Draw()
{
	bool drawToTarget = hasWavesInScreen();

	if( drawToTarget )
	{
		_screenCopy1->BeginRendering(Color::BLACK_TRANSPARENT);
		if(EditorUtils::debug_scale)
		{
			Render::device.PushMatrix();
			IPoint pos = Core::mainInput.GetMousePos() - GameSettings::FIELD_SCREEN_CONST.LeftBottom();
			//pos.y = GameSettings::FIELD_SCREEN_CONST.Height() - pos.y;
			Render::device.MatrixTranslate(pos);
			Render::device.MatrixScale(GameField::gameField->_debug_scale);
			Render::device.MatrixTranslate(-pos);
		}
		DrawField();
		if(EditorUtils::debug_scale)
		{
			Render::device.PopMatrix();
		}
		_screenCopy1->EndRendering();

		renderWaves();
	}

	if(GameSettings::isGlobalScaleExist)
	{
		Render::device.PushMatrix();
		Render::device.MatrixScale(GameSettings::scaleGlobal.x, GameSettings::scaleGlobal.y, 1.f);
	}
	if( drawToTarget ) {
		//Render::device.SetBlend(false);
		_screenCopy1->Draw(IPoint(0,0));
		//Render::device.SetBlend(true);
	} else {
		DrawField();
	}

	if(GameSettings::isGlobalScaleExist)
	{
		Render::device.PopMatrix();
	}
}

// рисует подсветку под теми фишками, которые будут уничтожены при взрыве текущей цепочки
void GameField::DrawSequenceAffectedZone() 
{
	if(_chipSeqAffectedVisual.empty()){
		return;
	}
	Place2D::SetParams(0.5f, 0.4f, 0.4f);
	Place2D::Clear();
	for(std::map<Game::FieldAddress, bool>::iterator itr = _chipSeqAffectedVisual.begin(); itr != _chipSeqAffectedVisual.end(); ++itr)
	{
		if(itr->second)
		{
			Place2D::AddAddress(itr->first);
		}
	}
	Place2D::CalculateWithoutVisability();

	//СШ:: если бонус взрывается бустом то тут ломалось, очевидно потому что цепочки никакой нет
	//Assert(GameSettings::chip_settings.find(_chipSeqColor) !=  GameSettings::chip_settings.end());

	Place2D::misc_borders.clear();
	Place2D::BindBorders(&Place2D::misc_borders);
	Color color_zone = Color::WHITE;
	if (_affectedZonePulsation) {
		color_zone.alpha = math::lerp(0, 180, _timerHideAffectedZone);
		Place2D::DrawPlaceWithoutVisability(Color::WHITE, GameSettings::CELL_RECT, 0.f, "select");
		Place2D::DrawBorders(Place2D::misc_borders, "select", color_zone);
		if (color_zone.alpha >= 180) {
			float alpha_timer = math::clamp(0.f, 1.f, 0.5f-0.5f*cosf(math::PI*_affectedZonePulsationTimer*2.0f));
			float add_alpha = math::lerp(0, 90, alpha_timer);
			Color color_zone_add = Color::WHITE;
			color_zone_add.alpha = add_alpha;
			Render::device.SetBlendMode(Render::ADD);
			Place2D::DrawPlaceWithoutVisability(Color::WHITE, GameSettings::CELL_RECT, 0.f, "select");
			Place2D::DrawBorders(Place2D::misc_borders, "select", color_zone_add);
			Render::device.SetBlendMode(Render::ALPHA);
		}
	} else {
		color_zone.alpha = math::lerp(0, 255, _timerHideAffectedZone);
		Place2D::DrawPlaceWithoutVisability(Color::WHITE, GameSettings::CELL_RECT, 0.f, "select");
		Place2D::DrawBorders(Place2D::misc_borders, "select", color_zone);
	}
}

void GameField::DrawUp()
{
	_endLevel.Draw();

	for(GameFieldController *ctrl : _controllers)
	{
		if(ctrl->z == 1) {
			ctrl->DrawAbsolute();
		}
	}

	// фишки, которые нужно нарисовать над туториальным затемнением
	FPoint offset = GameSettings::ToFieldPos(FPoint(0,0));
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(-offset);
	DrawChips(3);
	Render::device.PopMatrix();

	for(GameFieldController *ctrl : _controllers)
	{
		if(ctrl->z == 2) {
			ctrl->DrawAbsolute();
		}
	}
	for(GameFieldController *ctrl : _controllers)
	{
		if(ctrl->z == 3) {
			ctrl->DrawAbsolute();
		}
	}
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(-offset);
	_effContUpField.DrawBlend();
	Render::device.PopMatrix();

	_effContUp.DrawBlend();


	if (!EditorUtils::editor)
	{
		_tooltip.Draw();
	}
	_orderTooltip.Draw();

#ifndef  __APPLE__
	if(gameInfo.IsDevMode())
	{
		DrawDebugInfo();
	}
#endif


	//////!!! временно, потом удалить.  выводим отладочное сообщение с числом ходов///////
	//Render::FreeType::BindFont("debug");
	//Render::device.PushMatrix();
	//Render::device.MatrixTranslate(math::Vector3(20.f, 70.f, 0.f));
	//Render::PrintString(IPoint(70, 0), _movesToFinish);
	//Render::device.PopMatrix();

	/////////////////////////////////////
}

void GameField::DrawDebugInfo()
{
	if(!EditorUtils::draw_debug)
	{
		return;
	}

	Render::FreeType::BindFont("debug");

	Render::device.PushMatrix();
	{
		Render::device.SetTexturing(false);
		Render::BeginColor(Color(100,100,100, 200));
		Render::DrawRect(IRect(15, 0, 150, 75));
		Render::EndColor();
		Render::device.SetTexturing(true);

		Render::device.MatrixTranslate(math::Vector3(20.f, 70.f, 0.f));

		Render::PrintString(IPoint(0, 5), "dist = " + Float::ToString(Gadgets::squareDist[GameSettings::underMouseIndex]) );
		Render::PrintString(IPoint(0, -5), "col = " + Int::ToString(GameSettings::underMouseIndex.x) );
		Render::PrintString(IPoint(0, -15), "row = " + Int::ToString(GameSettings::underMouseIndex.y) );

		// Название текущего cтиля уровня из файла FieldStyles.xml
		if(FieldStyles::current)
		{
			Render::PrintString(IPoint(0, -45), FieldStyles::current->name);
		}

		//Координаты на поле
		IPoint field_mouse_pos = GameSettings::ToFieldPos(Core::mainInput.GetMousePos());
		Render::PrintString(IPoint(80, -25), "field mouse:");
		Render::PrintString(IPoint(105, -35), utils::lexical_cast(field_mouse_pos.x), 1.f, RightAlign);
		Render::PrintString(IPoint(132, -35), utils::lexical_cast(field_mouse_pos.y), 1.f, RightAlign);
	}
	Render::device.PopMatrix();
	
	{ //Показывает первые фишки которые должны начинать последовательность, но еще не одобрены
		Render::device.SetTexturing(false);
		Render::BeginColor(Color::WHITE);
		for(std::list<Game::FieldAddress>::iterator i = _firstChipsInChain.begin(); i != _firstChipsInChain.end(); i++)
		{
			IRect rect = GameSettings::CELL_RECT.MovedTo(GameSettings::ToScreenPos(i->ToPoint()*GameSettings::SQUARE_SIDE));
			rect.Inflate(-10);
			Render::DrawFrame(rect);
		}
		Render::EndColor();
		Render::device.SetTexturing(true);
	}
}

void GameField::DrawEditorChips()
{
	Render::SpriteBatch batch;
	batch.Begin(Render::SpriteSortMode::Deferred, Render::SpriteTransformMode::Auto);

	for(int x = Game::visibleRect.LeftBottom().x; x <= Game::visibleRect.RightTop().x; x++)
	{		
		for (int y = Game::visibleRect.LeftBottom().y; y <= Game::visibleRect.RightTop().y; y++)
		{
			if (x >= 200 || y >= 200 ||
				x < 0 || y < 0) {
				continue;
			}

			Game::Square *s = GameSettings::gamefield[x+1][y+1];
			if( !Game::isBuffer(s) )
			{
				s->DrawEditor(&batch); // Вот тут вcе cамое интереcное :)
			}
			else if( s == Game::bufSquareNoChip )
			{
				Render::device.SetTexturing(false);
				Render::BeginColor(Color::BLUE);
				FRect rect(Game::GetCellRect(Game::FieldAddress(x, y)));
				rect.Inflate(-15.0f);
				Render::DrawLine(rect.LeftBottom(), rect.RightTop());
				Render::DrawLine(rect.LeftTop(), rect.RightBottom());
				Render::EndColor();
				Render::device.SetTexturing(true);
			}
			else if( s == Game::bufSquareNoLicorice )
			{
				IPoint pos( Game::GetCenterPosition(Game::FieldAddress(x, y)) );
				Render::FreeType::BindFont("editor");
				Render::BeginColor(Color::RED);
				Render::PrintString(pos, "L", 2.0f, CenterAlign, CenterAlign, false);
				Render::EndColor();
			}
			else if( s == Game::bufSquareNoTimeBomb )
			{
				IPoint pos( Game::GetCenterPosition(Game::FieldAddress(x, y)) );
				Render::FreeType::BindFont("editor");
				Render::BeginColor(Color::RED);
				Render::PrintString(pos, "B", 2.0f, CenterAlign, CenterAlign, false);
				Render::EndColor();
			}
			else if( s == Game::bufSquareNoBomb )
			{
				IPoint pos( Game::GetCenterPosition(Game::FieldAddress(x, y)) );
				Render::FreeType::BindFont("editor");
				Render::BeginColor(Color::RED);
				Render::PrintString(pos, "HB", 2.0f, CenterAlign, CenterAlign, false);
				Render::EndColor();
			}
		}
	}

	batch.End();
}

void GameField::DrawSquares()
{
	for (int y = Game::visibleRect.RightTop().y; y >= Game::visibleRect.LeftBottom().y; y--)
	{
		for (int x = Game::visibleRect.LeftBottom().x - 1; x <= Game::visibleRect.RightTop().x + 1; x++)
		{
			Game::Square *s = GameSettings::gamefield[x+1][y+1];
			if(Game::isVisible(s) || s->IsShortSquare())
			{
				s->DrawSquare();
			}
		}
	}
}

bool ChipInRect(Game::Square *sq, IRect rect)
{
	FPoint chipPos = (sq->GetCellPos() + sq->GetChip().GetPos()) / GameSettings::SQUARE_SIDEF;
	IRect chipRect((int)chipPos.x, (int)chipPos.y, 1, 1);
	return rect.Intersects(chipRect);
}

void GameField::UpdateVisibleChips()
{
	for(size_t i = 0; i < 4; i++)
	{
		_squares_layer[i].clear();
	}
	for (int y = Game::visibleRect.RightTop().y; y >= 0; y--)
	{
		for (int x = Game::visibleRect.LeftBottom().x - 1; x <= Game::visibleRect.RightTop().x + 1; x++)
		{
			Game::Square *s = GameSettings::gamefield[x+1][y+1];
			if((Game::isVisible(s) && (s->IsStayFly() || EditorUtils::editor)) || s->IsShortSquare())
			{
				if( Game::visibleRect.Contains(IPoint(x,y)))
				{
					_squares_layer[0].push_back(s);
					_squares_layer[2].push_back(s);
					if(s->GetChip().HasDrawHang(s->address)) 
					{
						if(s->GetChip().IsChainHighlighted())
						{
							_squares_layer[3].push_back(s);
						}else{
							_squares_layer[1].push_back(s);	
						}
					}					
					
				}else if(ChipInRect(s, Game::visibleRect))
				{
					//Рисуем если фишка находится в зоне видимости
					_squares_layer[2].push_back(s);
				}
			}
		}
	}
}

void GameField::DrawChips(int layer)
{
	if(layer < 0 || layer > 3)
	{
		return;
	}
	Render::SpriteBatch batch;
	batch.Begin(Render::SpriteSortMode::Deferred, Render::SpriteTransformMode::Auto);
	for(auto sq : _squares_layer[layer])
	{
		sq->DrawChip(layer, &batch);
	}
	batch.End();
}


void GameField::SetEnergyShaderConstants()
{
	ShaderMaterial::ShaderParameters& vsParam = _energyShaderMaterial->GetVSParameters();
	ShaderMaterial::ShaderParameters& psParam = _energyShaderMaterial->GetPSParameters();

	float times[4] = {_localTime * 0.5f, 0.0f, 0.0f, 0.0f};
	vsParam.setParam("time", times, 4);

	float fieldPos[4] = { GameSettings::fieldX / 256.0f, GameSettings::fieldY / 256.0f, GameSettings::FIELD_SCREEN_CONST.width / 256.0f, GameSettings::FIELD_SCREEN_CONST.height / 256.0f};
	vsParam.setParam("field_pos", fieldPos, 4);

    psParam.setParam("max_color", _energyColor, 4);
}

void GameField::DrawEnergy()
{
	Render::device.SetStencilTest(true);
	 GameField::StartStencilWrite(0xC0, 0xF0);

	SetEnergyShaderConstants();

	Render::device.SetBlend(false);
	_energyShaderMaterial->Bind();

	Energy::field.Draw();

	_energyShaderMaterial->Unbind();
	Render::device.SetBlend(true);

	Render::device.SetTexturing(false);
	Energy::field.DrawFront();
	Render::device.SetTexturing(true);

	Render::device.SetStencilTest(false);
}

void GameField::UpdateEnergy(float dt)
{
	// пауза, с которой появляется энергия при старте уровня
	if (_energyPaused)
		return;
	else
	{
		_energyResumeDelay -= dt;
		if (_energyResumeDelay < 0.0f)
			_energyResumeDelay = 0.0f;
		if (_energyResumeDelay > 0.0f)
			return;
	}

	Energy::field.Update(dt * _energyTimeScale * _energyDistanceTimeScale);
}

void GameField::ResetEnergyLightTimer()
{
	_energyLightTimer = -gameInfo.getConstFloat("EnergyLightTimer", 1.f);
}


void GameField::ResetTimersAfterAction()
{
	if(_energyLightTimer < 0)
	{
		ResetEnergyLightTimer();
	}
}

void GameField::ClearChipSeq(const bool is_destroying)
{
	if(!_firstChipsInChain.empty())
	{
		for(auto i:_firstChipsInChain)
		{
			GameSettings::gamefield[i]->GetChip().Deselect();
		}
		_firstChipsInChain.clear();
	}
	_realFirstChipTime = -10.f;

	gameInfo.setLocalInt("FILL_bonus_current_s_count", 0);

	int chipSeqCount = (int) _chipSeq.size();

	if (chipSeqCount >= Gadgets::levelSettings.getInt("Seq0") && chipSeqCount > 0)
	{
		// Еcли длина цепочки не меньше, чем длина _longChain0, то
		// чаcть фишек, вокруг поcледней фишки, дрожит. При очиcтке 
		// цепочки дрожащие фишки нужно вернуть на cвои меcта

		const Game::FieldAddress& p = _chipSeq.back();

		for (int i = -3; i <= 3; i++)
		for (int j = -3; j <= 3; j++)
		{
			Game::FieldAddress p2 = p + Game::FieldAddress(i, j);
			if (!GameSettings::InField(p2))
			{
				continue;
			}

			Game::Square *s = GameSettings::gamefield[p2];

			s->GetChip().ClearOffset();
		}
	}

	// Еcли длина цепочки больше двух, то фишки cамой 
	// цепочки начинают дрожать. При очиcтке цепочки 
	// их тоже нужно вернуть на cвои меcта
	for (int i = 0; i < chipSeqCount; i++)
	{
		Game::Square *s = GameSettings::gamefield[_chipSeq[i]];
		s->GetChip().ClearOffset();
	}

	ResizeChipSeq(0, is_destroying);
}

void GameField::ResizeChipSeq(size_t start, const bool is_destroying)
{
	size_t size_chip_seq = _chipSeq.size();
	Assert(start <= size_chip_seq);

	// Снимаем выделение c ячеек:
	for (size_t i = start; i < _chipSeq.size(); ++i) {
		Game::Square* s = GameSettings::gamefield[_chipSeq[i]];
		s->GetChip().PlaySelected(false);

		if (i == 0)
			s->setFirstHighlight(false);

		if(!is_destroying)
			s->GetChip().Deselect();

		s->GetChip().ClearOffset();
	}
	_chipSeq.resize(start);
	size_chip_seq = _chipSeq.size();
	if (size_chip_seq == 1){
		GameSettings::gamefield[_chipSeq[0]]->setFirstHighlight(true);
	}
	if(!is_destroying)
	{
		_bonusCascade.clear();
		// просчитываем "зону поражения" для новой цепочки
		CalcSequenceEffects();
	}
	if(!_chipSeq.empty())
	{
		//Анимируем последнюю фишку
		GameSettings::gamefield[_chipSeq[_chipSeq.size()-1]]->GetChip().PlaySelected(true);

		//проигрываем звук		
		int chipsCount = _chipSeq.size();
		if (chipsCount > 11)
		{
			chipsCount = 11;
		}
		if(gameInfo.IsDevMode() && Core::mainInput.IsShiftKeyDown()) {
			MM::manager.PlaySample("AddChipInChain1", false, 1.f,  chipsCount * 0.1f + 0.5f);
		} else {
			MM::manager.PlaySample("AddChipInChain" + utils::lexical_cast(chipsCount));
		}

	}

	// убираем или показываем эффект над последней фишкой в длинной (бонусной) цепочке
	int count_bonus = UpdateBonusSequenceEffects();
	
	if( Gadgets::levelSettings.getString("FILL_ItemType") == "Fires" ) {
		gameInfo.setLocalInt("FILL_bonus_current_s_count", count_bonus);
	} else {
		gameInfo.setLocalInt("FILL_bonus_current_s_count", _chipSeq.size());
	}
}

int GameField::UpdateBonusSequenceEffects()
{
	int count_bonus = 0;

	std::string prefix = (Gadgets::levelSettings.getBool("FILL_bonus_allow") && Gadgets::levelSettings.getString("FILL_ItemType") == "Fires") ? "LSeq" : "Seq";

	if( Gadgets::levelSettings.getString("BonusRepeat") == "true" )
	{
		int period = Gadgets::levelSettings.getInt(prefix + "0");
		count_bonus = _chipSeq.size() / period;
	}
	else
	{
		for(size_t i = 0; i < 3; i++)
		{
			int seq_size = Gadgets::levelSettings.getInt(prefix + utils::lexical_cast(i));
			if(seq_size > 0 && seq_size <= (int)_chipSeq.size())
			{
				count_bonus++;
			}
		}
	}

	// огоньки кружащиеся вокруг конца цепочки
	if( count_bonus > 0 && prefix == "LSeq")
	{
		FPoint pos = FPoint(_chipSeq.back().GetCol() + 0.5f, _chipSeq.back().GetRow() + 0.5f) * GameSettings::SQUARE_SIDEF;
		if(!_seqFires) {
			_seqFires = new FlyFiresEffect(this, GameSettings::ToScreenPos(pos));
			Game::AddController(_seqFires);
		}
		_seqFires->SetFires(count_bonus);
		_seqFires->FlyToPos(GameSettings::ToScreenPos(pos));
	}
	else if(_seqFires)
	{
		_seqFires->SetFires(0);
		_seqFires = NULL;
	}

	return count_bonus;
}

int	GameField::getBonusLevelForSeq(const AddressVector& seq) const
{
	int length = 0;

	if( Gadgets::levelSettings.getString("BonusMethod") == "Lights" ){
		// бонус зависит от кол-ва светлячков в цепочке
		for(AddressVector::const_iterator itr = seq.begin(); itr != seq.end(); ++itr)
		{
			Game::Square *sq = GameSettings::gamefield[*itr];
			if(sq->GetChip().IsLight())
				++length;
		}
	} else {
		// бонус зависит от длины цепочки
		length = seq.size();
	}

	int r = 0;
	if( Gadgets::levelSettings.getString("BonusRepeat") == "true" )
	{
		if(length >= Gadgets::levelSettings.getInt("Seq0"))
			r = 1;
	}
	else
	{
		if (length >= Gadgets::levelSettings.getInt("Seq2")) {
			r = 3;
		} else if (length >= Gadgets::levelSettings.getInt("Seq1")){
			r = 2;
		} else if (length >= Gadgets::levelSettings.getInt("Seq0")) {
			r = 1;
		}
	}
	return (r);
}

std::string GameField::getBonusType(int bonusLevel, bool startight) const
{
	if(bonusLevel > 0){
		std::string tstr = startight ? "Type" : "DType";
		return Gadgets::levelSettings.getString(tstr + utils::lexical_cast(math::clamp(0, 2, bonusLevel-1)));
	} else {
		return "None";
	}
}

// бонус, который будет активирован на конце текущей цепочки (стрелки + бомба)
Game::Hang GameField::getCurrentSequenceBonus(AddressVector &checked_seq)
{
	Game::Hang seqHang;

	if (_useBoostLittleBomb && !_chipSeq.empty()) //если включен соответствующий буст, добавляем бомбочку
	{
		seqHang.Add(_globalHang, true);
	}

	// сумма висящих на цепочке бонусов (стрелок)
	if((Gadgets::levelSettings.getString("CombineBonusChain") == "true") || IsBonusComboSequence(checked_seq))
	{
		for(AddressVector::iterator itr = checked_seq.begin(); itr != checked_seq.end(); ++itr )
		{
			Game::Square *sq = GameSettings::gamefield[*itr];
			seqHang.Add(sq->GetChip().GetHang(), true);
		}
	}
	else
	{
		if( !checked_seq.empty() )
			seqHang.Add(GameSettings::gamefield[checked_seq.back()]->GetChip().GetHang(), true);
	}

	// добавим сюда бонус за длинную цепочку (бомбу), активируемый сразу
	if(Gadgets::levelSettings.getString("AutoTriggerBonus") == "true")
	{
		int bonusLevel = getBonusLevelForSeq(checked_seq);
		if( bonusLevel > 0 )
		{
			IPoint d = checked_seq[checked_seq.size()-1].ToPoint() - checked_seq[checked_seq.size()-2].ToPoint();
			bool straightSeq = (math::abs(d.x) + math::abs(d.y) == 1);
			std::string type = gameField->getBonusType(bonusLevel, straightSeq);
			std::string bonusChip = Gadgets::levelSettings.getString("ChainChipDieType");	
			std::string transform = Gadgets::levelSettings.getString("ChainChipTransform");
			Game::Hang::TransformChip tr = Game::Hang::NONE;
			if( transform == "chameleon" )
				tr = Game::Hang::CHAMELEON;
			else if( transform == "energy_bonus" )
				tr = Game::Hang::ENERGY_BONUS;
			IPoint dir(1, 0);
			std::string bonus_number = utils::lexical_cast(math::clamp(0, 2, bonusLevel-1));

			std::string rstr = straightSeq ? "Radius" : "DRadius";
			int radius = Gadgets::levelSettings.getInt(rstr + bonus_number);

			seqHang.Add(Game::Hang(type, radius, bonusLevel, IPoint(1,0), tr, true), true);
		}
	}

	return seqHang;
}

void GameField::UpdateChipSeq(float dt)
{
	// Если после матча цепочки будет взрыв, колеблем фишки в области взрыва
	Game::Hang hang = getCurrentSequenceBonus(_chipSeq);
	int r = getBonusLevelForSeq(_chipSeq);

	bool bombWillExplode = false;
	Game::BombBonus* bombBonus = (Game::BombBonus*)hang.GetBonusByType(Game::HangBonus::BOMB);
	if( bombBonus ) {
		bombWillExplode = true;
		r = bombBonus->GetRadius();
	}

	// Начинаем движение фишек цепочки, 
	// еcли в цепочке больше двух фишек
	bool sequenceChanged = false;
	int chipSeqCount = (int)_chipSeq.size();
	for (int i = 0; i < chipSeqCount; i++)
	{
		const Game::FieldAddress &p = _chipSeq[i];
		Game::Square *s = GameSettings::gamefield[p];

		// Еcли какие-то фишки в цепочке начали по 
		// какой-то причине падать, то убиваем цепочку
		if (!Game::isStandbyChip(p)) 
		{
			sequenceChanged = true;
			break;
		}
		
		// Подпрыгивание выделенных фишек
		bool is_ice = s->IsIce();
		FPoint cell_pos = s->GetCellPos();
		float offsetMul = s->IsIce() ? 0.1f : 1.0f;
		//if(chipSeqCount >= Gadgets::levelSettings.getInt("Seq1"))
		//{
		//	float t = GameSettings::fieldTimer * 18.0f + cell_pos.x * 0.35f + cell_pos.y * 0.5f;
		//	float x = 0.35f + 0.65f * math::sin(t);
		//	float xs = -0.3f * std::min(0.0f, x);
		//	s->GetChip().SetOffset(0.0f, 12.0f * std::max(0.0f, x) * offsetMul);
		//	if(!is_ice)
		//	{
		//		s->GetChip().SetScale(1.0f + xs, 1.0f - xs);
		//	}
		//}
		//else 
		if(chipSeqCount >= Gadgets::levelSettings.getInt("Seq0") && s->GetChip().IsFutureHang())
		{
			float t = GameSettings::fieldTimer * 14.0f + cell_pos.x * 0.35f + cell_pos.y * 0.5f;
			float x = 0.35f + 0.65f * math::sin(t);
			float xs = -0.25f * std::min(0.0f, x);
			s->GetChip().SetOffset(0.0f, 11.0f * std::max(0.0f, x) * offsetMul);
			if(!is_ice)
			{
				s->GetChip().SetScale(1.0f + xs, 1.0f - xs);
			}
		}else if(chipSeqCount >= 2)
		{
			float t = GameSettings::fieldTimer * 14.0f + cell_pos.x * 0.35f + cell_pos.y * 0.5f;
			float x = 0.35f + 0.65f * math::sin(t);
			float xs = -0.1f * std::min(0.0f, x);
			s->GetChip().SetOffset(0.0f, 2.0f * std::max(0.0f, x) * offsetMul);
			if(!is_ice)
			{
				s->GetChip().SetScale(1.0f + xs, 1.0f - xs);
			}
		}else
		{
			s->GetChip().ClearOffset();
		}
	}

	if (sequenceChanged)
	{
		// Убиваем цепочку и зануляем cмещения
		for (int i = 0; i < chipSeqCount; i++)
		{
			Game::Square *s = GameSettings::gamefield[_chipSeq[i]];
			s->GetChip().ClearOffset();
			s->GetChip().Deselect();
		}

		_sequenceStarted = false;
		
		ResizeChipSeq(0, false);
	}
}

// Зачем i и j могут быть меньше нуля?
bool GameField::isLineEmpty(int i, int j, int di, int dj, int size) const
{
	if (i < 0)
	{
		size += i*di;
		i = 0;
	}

	if (j < 0)
	{
		size += j*dj;
		j = 0;
	}

	for (int q = 0; q<size; q++)
	{
		// еcли выходят за границы поля
		if ((i > GameSettings::FIELD_MAX_WIDTH)||(j > GameSettings::FIELD_MAX_HEIGHT)) 
		{
			return true;
		}

		if (Game::isVisible(i, j))
		{
			return false;
		}

		// еcли cлева и cправа в радиуcе 2х еcть заполненные клетки, то cчитаем текущую также заполненной
		bool f1 = Game::isVisible(i-2,j) || Game::isVisible(i-1,j);
		bool f2 = Game::isVisible(i+1,j) || Game::isVisible(i+2, j);
		if (f1 && f2)
		{
			return false;
		}

		// еcли cнизу и cверху в радиуcе 2х еcть заполненные клетки, то cчитаем текущую также заполненной
		f1 = Game::isVisible(i, j-2) || Game::isVisible(i, j-1);
		f2 = Game::isVisible(i, j+1) || Game::isVisible(i, j+2);
		if (f1 && f2)
		{
			return false;
		}

		i += di;
		j += dj;
	}
	return true;
}


void GameField::ComputeFieldLimits()
{
	_fieldLeft = 0;
	while (isLineEmpty(_fieldLeft, 0, 0, 1, GameSettings::fieldHeight))
	{
		_fieldLeft++;
	}

	_fieldRight = GameSettings::fieldWidth - 1;
	while (isLineEmpty(_fieldRight, 0, 0, 1, GameSettings::fieldHeight))
	{
		_fieldRight--;
	}
	_fieldRight++;

	_fieldTop = GameSettings::fieldHeight-1;
	while (isLineEmpty(0, _fieldTop, 1, 0, GameSettings::fieldWidth))
	{
		_fieldTop--;
	}
	_fieldTop++;

	_fieldBottom = 0;
	while (isLineEmpty(0, _fieldBottom, 1, 0, GameSettings::fieldWidth))
	{
		_fieldBottom++;
	}
}


void GameField::OptimizeFieldPos(int &x, int &y, OptimizationReason &reason)
{
	reason = REASON_NO_OPTIMIZATION;

	if (Gadgets::receivers.FocusNeed())
	{	
		IPoint p = Gadgets::receivers.FocusCenter();
		x = p.x - GameSettings::FIELD_SCREEN_CENTER.x;
		y = p.y - GameSettings::FIELD_SCREEN_CENTER.y;
		reason = REASON_FOCUS_RECEIVER;
	}else 
	{
		// Проверяем активную в данный момент точку привязки
		const SnapGadget *gadget = Gadgets::snapGadgets.GetActiveGadget();

		if (gadget)
		{
			// Еcли еcть активная, то двигаем
			FPoint ps = gadget -> GetScrollPoint();
			reason = REASON_SNAP_GADGET;

			x = ps.x;
			y = ps.y;
		}
		else
		{
			//Данный вид выравнивания больше не используется.
			/*
			Правило №1
			Считается, что на уровне есть правильно расставленные точки привязки от начала и до конца! 
			Если такое условие по каким-то причинам не выполняется, то тогда наступает правило №2
			Правило №2
			Если на уровне нет ни одной точки привязки считается, что уровень помещается на экран iPhone(640x960) 
			полностью. Если точки привязки настроены неправильно, настройте их правильно или уберите.
			*/
			//reason = REASON_ENERGY_FLOW;
			//if(!CorrectFocusPosition(x, y))
			//{
			//	CorrectFieldPos(x,y);
			//}
		}
	}

}

void GameField::UpdateTooltip(float dt)
{
	//еcли уcловия для показа тултипа выполнены, то показываем на определённое время, иначе -- прячем;
	//в уcловия входит проверка на иcтечение времени показа

	// Тултип, плавающий за мышкой
	_tooltip.Update(dt);
	bool must_be_hidden = true, 
		visible = _tooltip.IsVisible(),
		fully_visible = _tooltip.IsFullyVisible(), 
		shown = _tooltip.ShowTimeIsExpired();
	std::string last_text_id = _tooltip.GetLastTextId();
	bool hiding = _tooltip.IsHiding();
	if (!_endLevel.IsRunning())
	{
		IPoint mouse_pos = Core::mainInput.GetMousePos();
		Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);
		// тултип для попытки некорректного применения бонуcа
		if (_toolInfoTextShow)
		{
			if (_toolAddress == index)
			{
				must_be_hidden = false;

				if (!fully_visible && !(shown && last_text_id == _toolInfoTextId))
					_tooltip.Show(_toolInfoTextId, 0.0f, 4.0f);

				if (shown && last_text_id == _toolInfoTextId)
				{
					must_be_hidden = true;
					_toolInfoTextShow = false;
					_toolAddress = Game::FieldAddress();
				}
			}
			else
			{
				_toolInfoTextShow = false;
				_toolAddress = Game::FieldAddress();
			}
		}	
	}

	if (must_be_hidden)
		_tooltip.HideSmoothly();

	if(_orderTooltip.IsVisible())
	{
	//	IPoint pos = _lastMouseScreenPos;
	//	pos.y += 80;
	//	pos.x -= _orderText->getWidth() / 2;
		_orderTooltip.SetPosExplicitly(_lastMouseScreenPos);
	}
	_orderTooltip.Update(dt);
}

void GameField::UpdateESourceAccent(float dt)
{
	if (!_sourceOpened || !_needESourceAccent || EditorUtils::editor || !_levelStarted)
	{
		return;
	}

	_eSourceAccentTime += dt;
	if (_eSourceAccentTime > 4.f)
	{
		_eSourceAccentTime = 0.f;
		std::vector<IPoint> vec;
		Gadgets::square_new_info.EnergySource_Get(vec);
		for(size_t i = 0; i < vec.size(); i++)
		{
			ParticleEffectPtr eff = Game::AddEffect(_effUnderChipsCont, "ESourceAccent");
			eff->posX = vec[i].x * GameSettings::SQUARE_SIDEF + GameSettings::SQUARE_SIDEF * 0.5f;
			eff->posY = vec[i].y * GameSettings::SQUARE_SIDEF + GameSettings::SQUARE_SIDEF * 0.5f;
			eff->Reset();
		}
	}
}

void GameField::RunWaveForChips(const int &current_color, const Game::FieldAddress &index)
{
	if(current_color >= 0)
	{
		Game::Square* last_square = GameSettings::gamefield[index];
		MyAssert(Game::isVisible(last_square));
		if(current_color >= 0)
		{
			if(last_square->GetChip().IsProcessedInSequence())
			{
				return;
			}
			Game::ChipColor::CURRENT_IN_SEQUENCE_ITER++;
			//Мой любимый обход в ширину
			std::vector<Game::FieldAddress> vec1, vec2, *vecPtr1, *vecPtr2;
			vecPtr1 = &vec1; vecPtr2 = &vec2;
			vecPtr1->push_back(_chipSeq[0]);
			while(!vecPtr1->empty())
			{
				vecPtr2->clear();
				size_t count = vecPtr1->size();
				for(size_t i = 0; i < count; i++)
				{
					GameSettings::gamefield[vecPtr1->at(i)]->GetChip().UpdateInSequence();
					for(BYTE k = 0; k < Gadgets::checkDirsChains.count; k++)
					{
						Game::FieldAddress a = vecPtr1->at(i) + Game::FieldAddress(Gadgets::checkDirsChains.dx[k], Gadgets::checkDirsChains.dy[k]);
						Game::Square* sq = GameSettings::gamefield[a];
						if(sq->GetChip().GetColor() == current_color && !sq->GetChip().IsProcessedInSequence())
						{
							vecPtr2->push_back(a);
						}
					}
				}
				std::swap(vecPtr1, vecPtr2);
			}
		}else{
			Game::ChipColor::chipSeqIsEmpty = true;
		}
	}
}

//находит клетки занятые растущими препятствиями высоты < 3 которые как следствие могут вырасти
void GameField::FindGrowingWood(AddressVector& squares)
{
	squares.clear();
	IPoint start = Game::activeRect.LeftBottom();

	for(int x = 0; x < Game::activeRect.Width(); ++x)
	{
		for(int y = Game::activeRect.Height()-1; y >= 0; --y)
		{
			Game::Square *sq = GameSettings::gamefield[start.x + x + 1][start.y + y + 1];
			if( sq->IsGrowingWood() && sq->GetWood() > 0 && sq->GetWood() < 3 ) {
				squares.push_back(sq->address);
			}
		}
	}
}


void GameField::UpdateSquares(float dt)
{
	Match3::FallColumnsOnUpdate();

	bool is_stand_by = IsStandby();
	bool need_collect_wall_squares = GameSettings::need_inc_wall_squares == 2 && is_stand_by;

	Game::chipsStandby = true;

	float rmin = -1.0f;

	int numSqWithEnergy = 0;
	std::vector<Game::Square*> growingWallSquares;
	std::vector<Game::Square*> groundCyclopsSquares;

	// Ниже код обновления фишек поля
	Game::ChipColor::chipSeqIsEmpty = _chipSeq.empty();
	Game::Square::HAS_FLYING_SQUARES = false;

	size_t squaresCount = GameSettings::squares.size();
	for (size_t i = 0; i < squaresCount; i++)
	{	
		Game::Square *s = GameSettings::squares[i];
		//if (Game::isBuffer(s)) continue;
		if (!Game::isVisible(s)) continue;

		bool visible = Game::visibleRect.Contains(s->address.ToPoint());
		bool active = Game::activeRect.Contains(s->address.ToPoint());

		s->Update(dt, visible, active);
		
		if(visible && need_collect_wall_squares && s->IsGrowingWall())
		{
			growingWallSquares.push_back(s);
		}

		if (visible && GameSettings::need_inc_ground_cycops && s->IsCyclops() && is_stand_by)
		{
			groundCyclopsSquares.push_back(s);
		}

		if (Energy::field.EnergyExists(s->address))
		{
			float squareDist = Gadgets::squareDist[s->address.GetCol()][s->address.GetRow()];
			if (rmin < 0.0f || rmin > squareDist)
			{
				_activeEnergy = s->address.ToPoint();
				rmin = squareDist;
			} 			
			numSqWithEnergy++;				
		}

		bool chipStandby = s->IsStandbyState();
		if (!chipStandby)
			Game::chipsStandby = false;

		if (!chipStandby)
			_noMovesTime = 0.0f;
	}

	if (numSqWithEnergy > 1)
	{
		_needESourceAccent = false;
	}

	// Растим землю
	if(!growingWallSquares.empty())
	{
		const Game::FieldAddress dirs[4] = {Game::FieldAddress::UP, Game::FieldAddress::DOWN, Game::FieldAddress::LEFT, Game::FieldAddress::RIGHT};
		size_t count = growingWallSquares.size();
		std::vector<Game::Square*> neighbours;
		for(size_t i = 0; i < count; i++)
		{
			Game::FieldAddress a = growingWallSquares[i]->address;
			for(size_t k = 0; k < 4; k++)
			{
				Game::Square *s = GameSettings::gamefield[a + dirs[k]];
				if(Game::isVisible(s) && s->GetWall() == 0 && !s->IsHardStand() && Game::activeRect.Contains(s->address.ToPoint()))
				{
					neighbours.push_back(s);
				}
			}
		} 
		count = neighbours.size();
		if(count > 0)
		{
			Game::Square *sq = neighbours[math::random(0u, count-1)];
			sq->SetWall(1);
			sq->SetGrowingWall(true);
			sq->AnimateGrowingWall();
			Energy::field.UpdateSquare(sq->address);
		}
		GameSettings::need_inc_wall_squares = 0;
	}

	//raise ground cyclops
	if (!groundCyclopsSquares.empty())
	{
		std::vector<Game::Square*> ground_cyclops_neighbours;
		size_t ground_cyclops_count = groundCyclopsSquares.size();
		const Game::FieldAddress dirs[4] = { Game::FieldAddress::UP, Game::FieldAddress::DOWN, Game::FieldAddress::LEFT, Game::FieldAddress::RIGHT };

		for (size_t i = 0; i < ground_cyclops_count; ++i)
		{
			Game::FieldAddress chip_address = groundCyclopsSquares[i]->address;
			for (size_t j = 0; j < 4; ++j)
			{
				Game::Square* square = GameSettings::gamefield[chip_address + dirs[j]];
				if (Game::isVisible(square) &&
					!square->IsHardStand() &&
					Game::activeRect.Contains(square->address.ToPoint()))
				{
					ground_cyclops_neighbours.push_back(square);
				}
			}
		}

		size_t ground_cyclops_neighbours_count = ground_cyclops_neighbours.size();
		if (ground_cyclops_neighbours_count > 0)
		{
			Game::Square *sq = ground_cyclops_neighbours[math::random(0u, ground_cyclops_neighbours_count - 1)];
			sq->SetCyclops(true);
			sq->GetChip().SetGroundCyclops();
			sq->ChangeCyclopsState(Game::Square::CYCLOPS_STATE_GROW);
			Energy::field.UpdateSquare(sq->address);
		}
		
		GameSettings::need_inc_ground_cycops = false;
	}
	//end raise ground cyclops

	if(_needUpdateSand && IsStandby())
	{
		UpdateSand();
	}

}

bool GameField::UpdateSand()
{
	_needUpdateSand = false;

	//static const IPoint sand_dirs[3] = {IPoint(0, -1), IPoint(-1, -1), IPoint(1, -1)};

	bool sandFallen = false;
	//for (int j = 2; j <= GameSettings::fieldHeight; j++)
	//{
	//	for (int i = 1; i <= GameSettings::fieldWidth; i++)
	//	{
	//		Game::Square *sq = GameSettings::gamefield[i][j];
	//		if( sq->IsSand() && sq->GetWood() == 0) // нашелся песочек
	//		{
	//			// ищем, куда ему можно упасть
	//			std::vector<IPoint> path;
	//			IPoint pos = sq->address.ToPoint();
	//			path.push_back(pos);
	//			bool find = true;
	//			while(find)
	//			{
	//				find = false;
	//				for(BYTE k = 0; k < 3; k++)
	//				{
	//					IPoint next = pos + sand_dirs[k];
	//					Game::Square *sq1 = GameSettings::gamefield[next];
	//					if( Game::isVisible(sq1) && sq1->GetWall() == 0 && !sq1->IsFake() && next != _energySource){
	//						find = true;
	//						path.push_back(next);
	//						pos = next;
	//						break;
	//					}					
	//				}
	//			}
	//			if(path.size()> 1)
	//			{
	//				Game::AddController(new Game::SandMover(path));
	//				sq->SetWall(0);
	//				GameField::gameField->PrepareSquareForEnergy(i-1, j-1);

	//				Game::Square *downSq = GameSettings::gamefield[path.back()];
	//				EnergyField::ClearEnergyInSquare(downSq);
	//				downSq->SetSand(true);
	//				sandFallen = true;
	//			}
	//		}
	//	}
	//}
	return sandFallen;
}


void GameField::Update(float dt)
{
	FieldStyles::current->fonDrawer.Update(dt);
	_effUnderChipsCont.Update(dt);
	_effCont.Update(dt);
	_effTopCont.Update(dt);
	_effContUp.Update(dt);
	_effContUpField.Update(dt);
	_timerEffCont.Update(dt);
	//_effContUpSquare.Update(dt);
	UpdateEnergyEffectContainer(dt);
	//_selectionChip.Update(dt);

	if(_timerHideAffectedZone < 1 && (!_bonusCascade.empty() || !_currentActiveBoost.expired()))
//	if(_timerHideAffectedZone < 1 && !_chipSeqAffectedVisual.empty())
	{
		_timerHideAffectedZone += dt*4.f;
		if(_timerHideAffectedZone >= 1)
		{
			_timerHideAffectedZone = 1.f;
		}
	}else if(_timerHideAffectedZone > 0 && _bonusCascade.empty() && _currentActiveBoost.expired())
	{
		_timerHideAffectedZone -= dt*4.f;
		if(_timerHideAffectedZone <= 0)
		{
			_chipSeqAffectedVisual.clear(); 
			_timerHideAffectedZone = 0.f;
		}
	}

	if (_affectedZonePulsation && _timerHideAffectedZone >= 1) {
		_affectedZonePulsationTimer += dt;
		if (_affectedZonePulsationTimer >= _affecredZonePulseTime) {
			_affectedZonePulsationTimer = 0.f;
		}
	}

	if (!EditorUtils::editor) {
		// этот вызов должен предшеcтвовать Gadgets::snapGadgets.Update(dt), иначе камеру унеcёт в cторону (0,0)
		if((_sourceOpened || _introController) && !Receiver::ReceiverEffect::keep_camera){
			CheckFieldMove();
		}
	}

	Receiver::ReceiverEffect::keep_camera = false;

	if (EditorUtils::editor)
	{
		if (!_levelScopeUpdated)
		{
			_levelScopeUpdateTime += dt;
			if (_levelScopeUpdateTime > 2.0f)
			{
				UpdateGameField(true);

				_levelScopeUpdated = true;
				_levelScopeUpdateTime = 0.f;
			}
		}
	}

	Game::timeFromLastBreakingWallSound += dt;
	
	if (!IsPaused())
	{
		UpdateESourceAccent(dt);
		if(_realFirstChipTime > 0)
		{
			_realFirstChipTime -= dt;
		}
		if(!_firstChipsInChain.empty() && _chipSeq.empty() && (_currentActiveBoost.expired()) && _firstChipsInChain.back().ToPoint() == GameSettings::underMouseIndex && !Game::Square::HAS_FLYING_SQUARES)
		{
			_moveWaitTimer += dt;
			if(_moveWaitTimer >= _moveWaitFull)
			{
				SequenceStart(GameSettings::gamefield[_firstChipsInChain.back()], 2);
				_realFirstChipTime = -10.f;
				_realFirstChip.SetCol(-1); _realFirstChip.SetRow(-1);
			}
		}else{
			_moveWaitTimer = 0.f;
		}
	}

	UpdateTooltip(dt);

	UpdateScores(dt);

	if( !_fieldMoving && _fieldMovingTimer > 0)
	{
		_fieldMovingTimer -= dt*2.f;
		if(_fieldMovingTimer < 0)
		{
			_fieldMovingTimer = 0.f;
		}
	}
	else if(_fieldMoving && _fieldMovingTimer < 1)
	{
		_fieldMovingTimer += dt*2.f;
		if(_fieldMovingTimer > 1)
		{
			_fieldMovingTimer = 1.f;
		}
	}

	//float old_field_timer = GameSettings::fieldTimer2; //сохраняем значение таймера, чтобы засечь переход через секунду
	GameSettings::fieldTimer2 += dt;

	float add_energy = 0.f;
	bool paused_for_timers = !IsPaused() && !SelectingSequence();
	{
		//Таймерам нужно дать проиграть уже начавшийся эффект
		//Обновление таймера подсветки энергии
		if(_energyLightTimer > 0)
		{
			_energyLightTimer += dt*0.5f;
			if(_energyLightTimer < 1)
			{
				//float t = (1.f - math::cos(_energyLightTimer*math::PI*2.f*3.f)) * 0.04f;
				//t += 0.015f * math::sin(_energyLightTimer*math::PI);
				add_energy = 0.0f;//t;
			}else{
				ResetEnergyLightTimer();
			}
		}else if(paused_for_timers){
			_energyLightTimer += dt;
		}else{
			ResetEnergyLightTimer();
		}
	}
	//Заносим цвет энергии в предшейдерную переменную учитывая добавочную яркость
	_energyColor[0] =  math::clamp(0.f, 1.f, Energy::Settings::color.red / 255.0f + add_energy);
	_energyColor[1] =  math::clamp(0.f, 1.f, Energy::Settings::color.green / 255.0f + add_energy);
	_energyColor[2] =  math::clamp(0.f, 1.f, Energy::Settings::color.blue / 255.0f + add_energy);
	_energyColor[3] =  math::clamp(0.f, 1.f, Energy::Settings::color.alpha / 255.0f + add_energy);


	if (!IsPaused())
	{
		GameSettings::fieldTimer += dt;

		_timeFromLastMove += dt;

		if(_endMoveDelay > 0.0f) {
			_endMoveDelay -= dt;
			if(_endMoveDelay <= 0.0f)
				OnEndMove();
		}

		_endLevel.Update(dt);

		if(_reshuffleChipsFlying == 0 )
		{
			OnReshuffleEnd();
			_reshuffleChipsFlying = -1;
		}
	}

	if (!IsPaused() || (!_sourceOpened && _level == 0))
	{
		Gadgets::ryushki.Update(dt);
	}

	if (!IsPaused())
	{
		_localTime += dt;
		Island::Update(dt);
	}

	if (!EditorUtils::editor) 
	{
		if( !IsPaused() && !Game::Square::HAS_FLYING_SQUARES)
		{
			// Обновляем энергию с примерно фиксированной частотой
			static float rest_dt = 0.f;
			rest_dt += dt;
			const float iter_dt = 0.02f;
			while (rest_dt > iter_dt) {
				rest_dt -= iter_dt;
				UpdateEnergy(iter_dt);
			}
		}
	}

	if(_introController == 0 && !Game::Square::HAS_FLYING_SQUARES && !_sourceOpened && !EditorUtils::editor)
	{
		gameField->OpenEnergySource();
	}
	
	Game::ChipColor::timeAfterLastFallSound += dt;

	if (!IsPaused() && !EditorUtils::editor)
	{
		Gadgets::receivers.Update(dt);

		if( !_fieldFilling && _levelStarted) {
			Match3GUI::TimeCounter::UpdateTime(dt);
		}

		Game::has_destroying = false;
		 
        UpdateSquares(dt);
		
		Gadgets::ug_prises.Update(dt);
		Gadgets::lockBarriers.Update(dt);
		Gadgets::snapGadgets.Update(dt);
		Gadgets::energySpeedChangers.Update(dt);
		Gadgets::bombFields.Update(dt);
		Gadgets::gates.Update(dt);

		UpdateChipSeq(dt);

		if (!IsStandby())
		{
			_noMovesTime = 0.0f;
			_maybeMoveGadget.NeedUpdate();
		} 
		else
		{
			//обновляем монстриков когда все успокоилось
			_movingMonsters.Update(dt);

			// Еcли фишки оcтановилиcь, то нужно поиcкать 
			// возможные ходы. Ведь может быть, что ходов 
			// не будет, и нужно делать перетаcовку
			_maybeMoveGadget.Find();

			if(_fieldFilling && !Game::Square::HAS_FLYING_SQUARES)
			{
				_fieldFilling = false;
				UnblockField();
				Match3::SetRect(_fieldLeft, _fieldRight, _fieldBottom, _fieldTop);
				Tutorial::luaTutorial.AcceptMessage(Message("OnGameStart"));
				if(_levelObjective == LevelObjective::DIAMONDS) {
					Game::UpdateDiamondsCount();
				}
			}
		}

		_movingMonsters.UpdatePulse(dt);

		if(!_chipSeq.empty())
		{
			size_t i = 0;
			size_t count = _chipSeq.size();
			bool bad = false;
			for(; i < count; i++)
			{
				Game::Square *sq = GameSettings::gamefield[_chipSeq[i]];
				MyAssert(Game::isVisible(sq));
				if(!sq->GetChip().IsExist())
				{
					bad = true;
					break;
				}
				//sq->GetChip().Select(i+1);
			}
			if(bad)
			{
				//Убираем все от исчезнвушей фишки до конца
				_chipSeq.erase(_chipSeq.begin() + i, _chipSeq.end());
				CalcSequenceEffects();
			}
		}

		if (_sourceOpened && !_endLevel.IsRunning() && _maybeMoveGadget.IsEmpty() && IsStandby() && !Game::HasController("BoostRunFallController"))
		{
			// UpdateSquares вперед. Он определяет состояние IsStandby()
			// И после _maybeMoveGadget.Find();
			if (!_chipSeq.empty())
			{
				ClearChipSeq(false);
			}
			NoMovesReshuffle();
		}

		

		// Можно пороверить счетчик
		bool canUpdateLevelActCounter = !EditorUtils::editor && !_endLevel.IsRunning() && _levelStarted
		                        && IsStandby() && !Energy::field.EnergyIsFlowing() && !Game::Spirit::BonusIsFlying()
								&& !Gadgets::receivers.IsFlying() && !_movingMonsters.IsEatingReceiver()
								&& !Game::Square::HAS_FLYING_SQUARES;	

		if (canUpdateLevelActCounter)
		{
			if (Match3GUI::ActCounter::GetCounter() <= 0 ) {
				_endLevel.SetLoseLifeReason(LevelEnd::OUT_OF_MOVES);
			} else if (Match3GUI::TimeCounter::GetTime() <= 0.0f) {
				_endLevel.SetLoseLifeReason(LevelEnd::OUT_OF_TIME);
			}

			if( IsLevelObjectiveCompleted() )
			{
				_endLevel.Run();
			}
			else if(_endLevel.NeedRunLoseLife() )
			{
				ClearChipSeq(false);
				_endLevel.RunLoseLife();
			}
		}

		//!!!временно, потом удалить. считаем сколько ходов до завершения

//		if ((int) old_field_timer != (int) GameSettings::fieldTimer2 ) //не чаще раза в секунду

		//unsigned long int t = gameInfo.getGlobalTime();
		//if (t % 10 == 0)            //не чаще раза в секунду
		//{
		//	if (IsStandby() && _levelStarted && !isBlocked())
		//	{
		//		_movesToFinish =  "MovesToFinish: " +  utils::lexical_cast(_fuuuTester.CountMovesToFinish(3 , 0.5));
		//	}
		//}
		///////////////////////////////////////////////
	}

	updateWaves(IsPaused() ? 0.0f : dt);
	RenderToTargets();

	Match3GUI::FillBonus::Get().Update(dt);
	UpdateVisibleChips();
}

void GameField::OnStartMove()
{
	SetEnergyDistanceTimeScale(1.0f);
	Match3::Pause();
	Match3GUI::ActCounter::ChangeCounter(-1);
	_isCurrentMoveActive = true;
	BlockField(false);
	Tutorial::luaTutorial.AcceptMessage( Message("OnStartMove") );
}

void GameField::EndMove()
{
	_endMoveDelay = 0.5f;

	// навешиваем бонусы
	if(Gadgets::levelSettings.getString("AutoTriggerBonus") != "true")
	{
		int bonusLevel = getBonusLevelForSeq(_chipSeqCopy);
		if( bonusLevel > 0 )
		{
			const Game::FieldAddress last0 = _chipSeqCopy[_chipSeqCopy.size() - 2];
			const Game::FieldAddress last1 = _chipSeqCopy[_chipSeqCopy.size() - 1];

			IPoint d = (last0.ToPoint() - last1.ToPoint());
			bool straightChain = (math::abs(d.x) + math::abs(d.y) == 1);

			std::string type = getBonusType(bonusLevel, straightChain);
			std::string bonusChip = Gadgets::levelSettings.getString("ChainChipDieType");	
			std::string transform = Gadgets::levelSettings.getString("ChainChipTransform");
			Game::Hang::TransformChip tr = Game::Hang::NONE;
			if( transform == "chameleon" )
				tr = Game::Hang::CHAMELEON;
			else if( transform == "energy_bonus" )
				tr = Game::Hang::ENERGY_BONUS;
			IPoint dir(1, 0);
			std::string bonus_number = utils::lexical_cast(math::clamp(0, 2, bonusLevel-1));
			std::string rstr = straightChain ? "Radius" : "DRadius";
			int radius = Gadgets::levelSettings.getInt(rstr + bonus_number);
			int count = Gadgets::levelSettings.getInt("BCount" + bonus_number);

			Game::Hang hang(type, radius, bonusLevel, last1.ToPoint() - last0.ToPoint(), tr, false);

			if( !hang.IsEmpty() ) {
				HangBonus(bonusChip, hang, count);
			}
		}
	}
}

void GameField::OnEndMove()
{
	float energy_distance = Energy::field.CalcDistanceToFlow();
	float energy_speed = math::clamp(1.0f, 2.0f, energy_distance * 0.3f);
	SetEnergyDistanceTimeScale(energy_speed);


	// Восстанавливаем треснутые клетки
	for (size_t i = 0; i < GameSettings::squares.size(); i++)
	{
		GameSettings::squares[i]->OnEndMove();
	}

	if(GameSettings::need_inc_wall_squares == 1)
	{
		GameSettings::need_inc_wall_squares = 2;
	}

	//ростим восстанавливающиеся препятствия
	if(GameSettings::need_inc_growing_wood == 1)
	{
		AddressVector growingWood; 
		FindGrowingWood(growingWood); //ищем где они есть
		if (!growingWood.empty()) //если есть
		{
			//ищем сколько ростить
			int growPercent = Gadgets::levelSettings.getInt("GrowingWoodPercent");
			size_t growCount = growingWood.size() * static_cast<float>(growPercent) / 100;
			if (growCount == 0 ) { growCount = 1;} //как минимум один

			while (growCount>0 && growingWood.size() > 0) //пока есть чему расти
			{
				size_t count = growingWood.size();
				size_t n = math::random(0u, count-1);   //выбираем случайную клетку
				Game::Square *sq = GameSettings::gamefield[growingWood[n]];
				sq->SetWood(sq->GetWood()+1); //ростим

				//удаляем чтобы не растить 2 раза
				growingWood.erase(growingWood.begin() + n);

				--growCount;
			}
		}

		GameSettings::need_inc_growing_wood = 0;
	}

	Gadgets::receivers.OnEndMove();

	_needUpdateSand = true;

	Match3::Unpause();

	_isCurrentMoveActive = false;

	GameSettings::sequenceWithEnergySquare = false;

	Game::Orders::OnEndMove();

	UnblockField();

	_maybeMoveGadget.NeedUpdate();

	if (gameInfo.getConstBool("CollectStatistics"))
	{
		int moves = utils::lexical_cast<int>(playerStatistic.GetValue("SpentMoves"));
		playerStatistic.SetValue("SpentMoves", utils::lexical_cast(moves+1));

	}
	//увеличиваем запас ходов у монстриков. но фактически двинутся в Update
	++_movingMonsters._needMoveMonsters;
	//ход сделан и будить можно
	_movingMonsters._canWakeMonsters = true; 


	//Проверим дотечет ли энергия до какого-нибудь приемника энергии
	Gadgets::freeFrontDetector.Update();

	// рождаем алмазы
	if( Game::diamondsOnScreen < Gadgets::levelSettings.getInt("DiamondLimit") && math::random(0,99) < Gadgets::levelSettings.getInt("DiamondPercent") )
	{
		Game::Square *sq = Gadgets::chipSources.GetRandomDiamondSource();
		if( sq )
		{
			sq->GetChip().SetDiamond();
			Game::diamondsOnScreen++;
		}
	}
}

bool GameField::SelectingSequence()
{
	return !_chipSeq.empty() || !_firstChipsInChain.empty();
}


void GameField::CheckFieldMove()
{
	// еcли глаза меня не обманывают здеcь у наc обрабатываетcя движение камеры за энергией (camera move)
	if (EditorUtils::editor || _timeFromLastMove < 0.1f || _endLevel.IsRunning() || SelectingSequence())
	{
		return;
	}
	
	OptimizationReason reason = REASON_NO_OPTIMIZATION;

	OptimizeFieldPos(_destFieldPos.x, _destFieldPos.y, reason);

	if (_fieldMoving)
	{
		return;
	}

	float dx = GameSettings::fieldX - _destFieldPos.x;
	float dy = GameSettings::fieldY - _destFieldPos.y;

	if(sqrt(dx * dx + dy * dy) > 0.1f)
	{
		float timeScale = _energyTimeScale * _energyDistanceTimeScale;

		const SnapGadget *snap_gadget = Gadgets::snapGadgets.GetActiveGadget();
		if(snap_gadget) // && !snap_gadget->IsFix()){
		{
			timeScale = snap_gadget->GetScrollSpeed()*gameInfo.getGlobalFloat("SnapGadgetSpeedCoef", 0.0015f);
		}
		FieldMover *fm = new FieldMover(this, timeScale);
		Game::AddController(fm);

		Gadgets::gates.RemoveVisible();

		Game::UpdateTargetRect();

		_movingMonsters._canWakeMonsters = false; //нельзя будить так сразу
	}
	//else if(Energy::field.EnergyIsFlowing()){
	//	//Сообщаем фронту энергии что камера остановилась и не намеревается двигаться дальше пока течет энергия
	//	Gadgets::freeFrontDetector.SetCameraIsStand();
	//}
}

// запуcк эффекта взрыва бомбы на поле
void GameField::DoBigBomb(IPoint pos, int radius)
{
	Game::Hang bonus;
	bonus.MakeBomb(radius, 3);
	Game::AddController( new CombinedBonus(Game::FieldAddress(pos), bonus, this, false) );
}

void GameField::MoveBonusesToViewRect()
{
	if( Gadgets::levelSettings.getString("MoveBonusWithCamera") == "false") {
		//Проверять нечего пусть остаются на своих местах за экраном
		return;
	}

	std::vector<Game::Square*> bonuses;

	// ищем бонусы, которые нужно переместить
	size_t squaresCount = GameSettings::squares.size();
	for (size_t i = 0; i < squaresCount; i++)
	{	
		Game::Square *sq = GameSettings::squares[i];
		if( !Game::isBuffer(sq) && sq->GetChip().IsExist() && !Game::activeRect.Contains(sq->address.ToPoint()) && sq->GetChip().HasHang())
		{
			bonuses.push_back(sq);
		}
	}

	std::vector<Game::FieldAddress> available;
	Game::GetAddressesForHangBonus(available);

	size_t bonusNum = std::min(bonuses.size(), available.size());
	for(size_t i = 0; i < bonusNum; i++)
	{
		Game::Hang hang = bonuses[i]->GetChip().GetHang();
		Game::AddController(new Game::Spirit(GameSettings::ToScreenPos(FPoint(bonuses[i]->address.ToPoint())*GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF), IPoint(-1, -1), std::string("screen"), hang, AddressVector(), available[i], 0.f, bonuses[i]->GetChip().GetColor())); 
		bonuses[i]->GetChip().GetHang().Clear();
	}
}

void GameField::TriggerOffscreenBonuses()
{
	CombinedBonus::BonusCascade bonuses;

	// ищем бонусы, которые нужно взорвать
	size_t squaresCount = GameSettings::squares.size();
	for (size_t i = 0; i < squaresCount; i++)
	{	
		Game::Square *sq = GameSettings::squares[i];
		if( !Game::isBuffer(sq) && sq->GetChip().IsExist() && !Game::activeRect.Contains(sq->address.ToPoint()) && sq->GetChip().HasHang())
		{
			bonuses.push_back( std::make_pair(sq->address, sq->GetChip().GetHang()) );
		}
	}

	// взрываем
	Game::AddController(new CombinedBonus(bonuses, this, false, CombinedBonus::INSTANT));
}

void GameField::TriggerBonusesOnScreen()
{
	CombinedBonus::BonusCascade bonuses;

	// ищем бонусы, которые нужно взорвать
	size_t squaresCount = GameSettings::squares.size();
	for (size_t i = 0; i < squaresCount; i++)
	{	
		Game::Square *sq = GameSettings::squares[i];
		if( !Game::isBuffer(sq) && sq->GetChip().IsExist() && sq->GetChip().HasHang() && Game::activeRect.Contains(sq->address.ToPoint()))
		{
			bonuses.push_back( std::make_pair(sq->address, sq->GetChip().GetHang()) );
		}
	}

	// взрываем
	Game::AddController(new CombinedBonus(bonuses, this, false, CombinedBonus::RANDOM));
}

int GameField::HangBonusesFromMoves(int count)
{
	int bonusNum = std::min(count, Match3GUI::ActCounter::GetCounter());

	std::vector<Game::FieldAddress> chips;
	
	for (int x = Game::activeRect.LeftBottom().x; x <= Game::activeRect.RightTop().x; x++)
	{
		for (int y = Game::activeRect.LeftBottom().y; y <= Game::activeRect.RightTop().y; y++)
		{
			Game::Square *sq = GameSettings::gamefield[x+1][y+1];
			if(Game::isVisible(sq))
			{
				chips.push_back(sq->address);
			}
		}
	}
	std::random_shuffle(chips.begin(), chips.end());

	bonusNum = std::min(bonusNum, (int)chips.size());

	FPoint movesPos = Core::LuaCallFunction<FPoint>("gameInterfaceCounterPosition");
	AddressVector seq(2, Game::FieldAddress(0,0) );
	for(int i = 0; i < bonusNum; ++i)
	{
		Game::Hang bonus;
		if( math::random(0,1) )
			bonus.MakeArrow(11, Game::Hang::ARROW_U | Game::Hang::ARROW_D );
		else
			bonus.MakeArrow(11, Game::Hang::ARROW_L | Game::Hang::ARROW_R );
		Game::Spirit *spirit = new Game::Spirit(movesPos, IPoint(-1,-1), "screen", bonus, seq, chips[i]);
		spirit->SetAutoTrigger();
		Game::AddController(spirit);
	}

	Match3GUI::ActCounter::ChangeCounter(-bonusNum);

	return bonusNum;
}

void GameField::TriggerBonusesFromMoves()
{
	std::vector<Game::FieldAddress> chips;
	
	for (int x = Game::activeRect.LeftBottom().x; x <= Game::activeRect.RightTop().x; x++)
	{
		for (int y = Game::activeRect.LeftBottom().y; y <= Game::activeRect.RightTop().y; y++)
		{
			Game::Square *sq = GameSettings::gamefield[x+1][y+1];
			if(Game::isVisible(sq))
			{
				chips.push_back(sq->address);
			}
		}
	}
	std::random_shuffle(chips.begin(), chips.end());

	int bonusNum = std::min(Match3GUI::ActCounter::GetCounter(), (int)chips.size());

	int radius = std::max(0, 5 - bonusNum);

	CombinedBonus::BonusCascade cascade;
	for(int i = 0; i < bonusNum; ++i)
	{
		Game::Hang bonus;
		bonus.MakeBonus( boost::make_shared<LevelEndBonus>(math::random(0.0f, 1.5f), radius) );
		cascade.push_back( std::make_pair(chips[i], bonus) );
		//Game::AddController(new LevelEndFlyBonus(this, chips[i], bonus, 0.1f * i));
	}
	Game::AddController(new CombinedBonus(cascade, this, false, CombinedBonus::RANDOM));
}

void GameField::MouseDoubleClick(const IPoint &mouse_pos)
{
	if (GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos))
	{
		Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);
		Game::Square* sq = GameSettings::gamefield[index];

		if( gameInfo.IsDevMode() && Game::isVisible(sq) && sq->GetChip().IsSimpleChip()) {
			int colorIdx = (Gadgets::levelColors.GetIndex(sq->GetChip().GetColor()) + 1) % Gadgets::levelColors.GetCount();
			sq->GetChip().SetColor(Gadgets::levelColors[colorIdx]);
		}
	}
}

bool GameField::ProcessFirstClick(const IPoint &mouse_pos)
{
	_timerHideAffectedZone = 0.f; 
	Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);
	Game::Square* sq = GameSettings::gamefield[index];
	if (sq->IsNormal() && !sq->IsFake() && sq->GetChip().IsSimpleChip() && Game::activeRect.Contains(index.ToPoint())) // нажали на клетку c фишкой не может быть льдом		
	{
		if (!sq->GetChip().IsExist() || !sq->GetChip().IsStand()){
			return true;
		}

		// во время туториала позволяем выделять только то, что показано
		if(Game::ChipColor::tutorialChainLength > 0 && !sq->GetChip().IsChainHighlighted()) {
			return true;
		}

		_sequenceStarted = true;
		 
		if(!_chipSeq.empty())
		{
			//MyAssert(false);
			ClearChipSeq(false);					
		}
		_firstChipsInChain.clear();
		_moveWaitTimer = 0.f;

		// Начинаем новую цепочку...
		IPoint field_mp = GameSettings::ToFieldPos(mouse_pos);
		IPoint chipPos = index.Get() * GameSettings::SQUARE_SIDE + IPoint(GameSettings::SQUARE_SIDE/2, GameSettings::SQUARE_SIDE/2);
		bool hit_true = (abs(chipPos.x - field_mp.x + 0.f) + abs(chipPos.y - field_mp.y + 0.f)) < romb_square_scale; //проверяется область попадания в клетку

		_realFirstChip = index;
		_realFirstChipTime = GameSettings::SEQ_FAST_MOVE_TIME;
		if(hit_true) {
			//_indexFirst = index;
			_firstChipsInChain.push_back(index);
			GameSettings::gamefield[index]->GetChip().Select(1, index.ToPoint());
			if (_chipSeq.size() == 0 && Game::activeRect.Contains(index.ToPoint())) { //тут, чтобы воспроизвести первый звук без задержки
				MM::manager.PlaySample("AddChipInChain1");
			}
		} else {
			//Неточное попадание чревато для рядов фишек длинной 2
			//Проверяем сколько фишек можно собрать вместе с index
			if(Game::CheckMaxSeq(index, 3))
			{
				_firstChipsInChain.push_back(index);
			}
		}
	}
	return false;
}

bool GameField::MouseDown(const IPoint &mouse_pos)
{
	Tutorial::luaTutorial.AcceptMessage( Message("OnTap") );

	Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);
	GameSettings::underMouseIndex = index.ToPoint();

	if(_endLevel.MouseDown(mouse_pos))
	{
		return true;
	}


	if (Core::mainInput.GetMouseRightButton() && gameInfo.IsDevMode())
	{ //isBlocked - не влияет на возможность применения читов, но и читы не должны выкидывать return
		Game::Square* sq = GameSettings::gamefield[index];
		if(sq->GetChip().IsSimpleChip() )
		{
			if( Core::mainInput.IsControlKeyDown() )
			{
				// чит: меняем навешиваемый бонус на фишке
				const Game::Hang &hang = sq->GetChip().GetHang();
				Game::Hang newHang;
				if( hang.IsEmpty() ) {
					newHang.MakeArrow(11, Game::Hang::ARROW_L | Game::Hang::ARROW_R);
				} else if( Game::ArrowBonus *arrow = (Game::ArrowBonus*)hang.GetBonusByType(Game::HangBonus::ARROW) ) {
					if( arrow->GetDirections() == (Game::Hang::ARROW_L | Game::Hang::ARROW_R) ) {
						newHang.MakeArrow(11, Game::Hang::ARROW_U | Game::Hang::ARROW_D);
					} else {
						newHang.MakeBomb(1, 1);
					}
				} else if( hang.GetBonusByType(Game::HangBonus::BOMB) ) {
					newHang.MakeLightning(0);
				}
				sq->GetChip().SetHang(newHang);
			}else if(Core::mainInput.IsShiftKeyDown())
			{
				if(sq->IsFlyType(Game::Square::FLY_NO_DRAW))
				{
					sq->SetFlyType(Game::Square::FLY_APPEARING, false);
				}else if(sq->IsFlyType(Game::Square::FLY_STAY))
				{
					sq->SetFlyType(Game::Square::FLY_HIDING, false);
				}
			}
			else
			{
				if( Game::isVisible(sq) ) {
					int colorIdx = (Gadgets::levelColors.GetIndex(sq->GetChip().GetColor()) + 1) % Gadgets::levelColors.GetCount();
					sq->GetChip().SetColor(Gadgets::levelColors[colorIdx]);
				}
			}
		}
	} 

	if (isBlocked())
	{
		return false;
	}

	if (_endLevel.IsRunning())
	{
		return false;
	}


	GameFieldControllerList::iterator i = _controllers.begin(), e = _controllers.end();
	for (; i != e; i++)
	{
		if((*i)->MouseDown(mouse_pos))
		{
			return true;
		}
	}
	
	_mouseDown = true;

	if (!_levelStarted){
		_levelStarted = true;
		gameInfo.getGlobalBool("GameSaved", false);
	}

	if (GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos))
	{
		Game::Square* sq = GameSettings::gamefield[index];
		if (Core::mainInput.GetMouseRightButton())
		{
			_effCont.KillAllEffects();
		}
		Gadgets::ryushki.MouseDown(mouse_pos);
		if (Game::isBuffer(sq))
		{
			return true;
		}

		if (Core::mainInput.GetMouseRightButton())
		{
			_sequenceStarted = false;
			ClearChipSeq(false);
		} 
		else if (!EditorUtils::editor && Game::activeRect.Contains(index.ToPoint()) && Match3GUI::ActCounter::GetCounter() > 0 && Match3GUI::TimeCounter::GetTime() > 0.0f)
		{
			//
			// Не в редакторе и нажата левая кнопка мышки
			//
			return ProcessFirstClick(mouse_pos);
		}
	}
	return true;
}

void GameField::OnAddChipInChain(const Game::FieldAddress& address)
{
	Assert(!_chipSeq.empty());

	int chipsCount = _chipSeq.size();
	if (chipsCount > 11)
	{
		chipsCount = 11;
	}

	if(gameInfo.IsDevMode() && Core::mainInput.IsShiftKeyDown()) {
		MM::manager.PlaySample("AddChipInChain1", false, 1.f,  chipsCount * 0.1f + 0.5f);
	} else {
		if (chipsCount > 1) MM::manager.PlaySample("AddChipInChain" + utils::lexical_cast(chipsCount));
	}

	//если там есть бонус то пиликаем
	Game::Square *sq = GameSettings::gamefield[address];
	if (sq->GetChip().HasHang())
	{
		MM::manager.PlaySample("ArrowBonusInChain");
	}

	//Game::Square *sq = GameSettings::gamefield[_chipSeq.back()];
	//sq->GetChip().Select(_chipSeq.size(), sq->GetCellPos());

	ResetTimersAfterAction();
	if(_chipSeq.size() >= 2)
	{
		//Снять анимацию выделения предпоследней фшики
		size_t i = _chipSeq.size() - 2;
		GameSettings::gamefield[_chipSeq[i]]->GetChip().PlaySelected(false);
	}

	int count_bonus = UpdateBonusSequenceEffects();

	if( Gadgets::levelSettings.getString("FILL_ItemType") == "Fires" ) {
		gameInfo.setLocalInt("FILL_bonus_current_s_count", count_bonus);
	} else {
		gameInfo.setLocalInt("FILL_bonus_current_s_count", _chipSeq.size());
	}

	// просчитываем зону поражения для изменившейся цепочки
	CalcSequenceEffects();
	RunWaveForChips(_chipSeqColor, address);
}

void GameField::NoMovesReshuffle()
{
	if (_reshuffleChipsFlying >= 0 || EditorUtils::editor)
	{
		return;
	}

	if( ReshuffleField() )
	{
		BlockField();
	}
	else
	{
		_endLevel.SetLoseLifeReason(LevelEnd::NO_MOVES);
	}
}

// фишка на данном квадрате не может никуда деться при решаффле
bool CanReshuffleCell(Game::Square *sq)
{
	return !sq->IsIce() && !sq->GetChip().IsLock();
}

bool GameField::ReshuffleField()
{
	ParticleEffectPtr eff = Game::AddEffect(_effTopCont, "Reshuffle");
	eff->SetPos(GameSettings::GetCenterOnField());
	eff->Reset();

	_reshuffleChipsFlying = 0;

	// составляем списки всех клеток для перемешивания
	std::vector<Game::ChipColor> fromChips;
	std::vector<Game::Square*> fromCells;
	std::vector<Game::Square*> toCells;
	for(int x = 0; x < Game::activeRect.Width(); ++x)
	{
		for(int y = 0; y < Game::activeRect.Height(); ++y)
		{
			Game::FieldAddress fa(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if(sq->GetChip().IsSimpleChip() && CanReshuffleCell(sq))
			{
				fromCells.push_back(sq);
				toCells.push_back(sq);
				fromChips.push_back(sq->GetChip());
			}
		}
	}
	std::random_shuffle(toCells.begin(), toCells.end());

	// ищем все возможные ходы исходя из положения клеток и неподвижных фишек на них
	struct Move {
		int color;
		std::vector<Game::Square*> cells;
	};
	std::vector<Move> moves;
	for(int x = 0; x < Game::activeRect.Width(); ++x)
	{
		for(int y = 0; y < Game::activeRect.Height(); ++y)
		{
			Game::FieldAddress fa(Game::activeRect.x + x, Game::activeRect.y + y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if(sq->GetChip().IsSimpleChip())
			{
				// для каждого цвета перебираем соседей
				for(int i = 0; i < Gadgets::levelColors.GetCount(); ++i)
				{
					int color = Gadgets::levelColors[i];
					if(!CanReshuffleCell(sq) && !sq->GetChip().IsAdapt() && sq->GetChip().GetColor() != color)
						continue;

					int needNeighboursToMatch = 2;
					std::vector<Game::Square*> neighbours; // соседние клетки, куда мы можем что-нибудь положить
					for(size_t k = 0; k < Gadgets::checkDirsChains.count; ++k)
					{
						Game::FieldAddress nfa = fa.Shift(Gadgets::checkDirsChains.dx[k], Gadgets::checkDirsChains.dy[k]);
						Game::Square *nsq = GameSettings::gamefield[nfa];
						if(Game::activeRect.Contains(nfa.ToPoint()) && nsq->GetChip().IsSimpleChip())
						{
							if( CanReshuffleCell(nsq) ) {
								neighbours.push_back(nsq);
							} else if(nsq->GetChip().IsAdapt() || nsq->GetChip().GetColor() == color) {
								needNeighboursToMatch--;
							}
						}
					}

					if( (int)neighbours.size() >= needNeighboursToMatch )
					{
						Move move;
						move.color = color;
						if( CanReshuffleCell(sq) )
							move.cells.push_back(sq);

						std::random_shuffle(neighbours.begin(), neighbours.end());
						for(int i = 0; i < needNeighboursToMatch; ++i) {
							move.cells.push_back( neighbours[i] );
						}

						moves.push_back( move );
					}
				}
			}
		}
	}

	// смотрим сколько фишек каких цветов у нас есть
	std::map<int, std::vector<int> > cellColors; // <color <index> >
	for( size_t i = 0; i < fromCells.size(); ++i )
	{
		Game::Square *sq = fromCells[i];
		if( sq->GetChip().IsAdapt() ) {
			for(std::map<int, std::vector<int> >::iterator itr = cellColors.begin(); itr != cellColors.end(); ++itr)
				itr->second.push_back(i);
		} else {
			cellColors[sq->GetChip().GetColor()].push_back(i);
		}
	}

	// проверяем какие ходы можно сделать из имеющегося набора фишек
	for(std::vector<Move>::iterator itr = moves.begin(); itr != moves.end(); )
	{
		if( cellColors[itr->color].size() < itr->cells.size() ) {
			itr = moves.erase(itr);
		} else {
			++itr;
		}
	}

	if( !moves.empty() )
	{
		// выбираем подходящий ход
		size_t idx = math::random(0u, moves.size()-1);
		std::vector<Game::Square*> chainCells = moves[idx].cells;
		std::vector<int> chipCells = cellColors[moves[idx].color];
		std::random_shuffle(chipCells.begin(), chipCells.end());
		Assert( chipCells.size() >= chainCells.size() );
		chipCells.resize( chainCells.size() );

		// отправляем выбранные для цепочки фишки на выбранные клетки
		for(size_t i = 0; i < chipCells.size(); ++i)
		{
			int index = chipCells[i];
			Game::ChipColor &chip = chainCells[i]->GetChip();
			chip = fromChips[index];
			chip.ClearOffset();
			FPoint pos = chip.GetPos() + fromCells[index]->GetCellPos() - chainCells[i]->GetCellPos();
			chip.SetPos(pos);

			Game::AddController(new ReshuffleChipFly(chip, chainCells[i]));
			_reshuffleChipsFlying++;
		}

		// удаляем обработанные клетки из fromCells, fromChips
		std::sort(chipCells.begin(), chipCells.end(), std::greater<int>()); // сортируем индексы в обратном порядке, чтобы при удалении последующие индексы оставались валидными
		for(size_t i = 0; i < chipCells.size(); ++i)
		{
			int index = chipCells[i];
			fromCells.erase( fromCells.begin() + index );
			fromChips.erase( fromChips.begin() + index );
		}

		// удаляем обработанные клетки из toCells
		for(std::vector<Game::Square*>::iterator itr = chainCells.begin(); itr != chainCells.end(); ++itr)
		{
			std::vector<Game::Square*>::iterator rm_itr = std::find(toCells.begin(), toCells.end(), *itr);
			if( rm_itr != toCells.end() )
				toCells.erase(rm_itr);
			else
				Assert(false);
		}

		Assert( fromCells.size() == toCells.size() );
		Assert( fromChips.size() == toCells.size() );
	}

	// перемешиваем все остальное
	for(size_t i = 0; i < toCells.size(); ++i)
	{
		Game::ChipColor &chip = toCells[i]->GetChip();
		chip = fromChips[i];
		chip.ClearOffset();
		FPoint pos = chip.GetPos() + fromCells[i]->GetCellPos() - toCells[i]->GetCellPos();
		chip.SetPos(pos);

		Game::AddController(new ReshuffleChipFly(chip, toCells[i]));
		_reshuffleChipsFlying++;
	}

	return !moves.empty();
}

void GameField::OnReshuffleEnd()
{
	UnblockField();
	_maybeMoveGadget.NeedUpdate();
	_maybeMoveGadget.Find();
}

void GameField::UpdateGameField(bool needUpdateLevelScope)
{
	ComputeFieldRect();

	size_t count = GameSettings::squares.size();
	for (size_t i = 0; i < count; i++)
	{
		Game::Square *s = GameSettings::squares[i];
		GameSettings::gamefield[s->address] = s;
	}

	if (needUpdateLevelScope)
	{
		levelBorder.Calculate();
	}
}

void GameField::MouseWheel(int delta)
{
	if(EditorUtils::debug_scale)
	{
		_debug_scale = math::clamp(1.f, 100.f, _debug_scale + delta); 
	}

}

void GameField::MouseCancel()
{
	ClearChipSeq(false);
}

void GameField::MouseUp(const IPoint &mouse_pos)
{
	Tutorial::luaTutorial.AcceptMessage( Message("OnTap") );

	_realFirstChip.SetCol(-1);
	_realFirstChip.SetRow(-1);

	GameFieldControllerList::iterator i = _controllers.begin(), e = _controllers.end();
	for (; i != e; i++)
	{
		if((*i)->MouseUp(mouse_pos))
		{
			return;
		}
	}

	if (isBlocked())
	{
		return;
	}

	if(SelectingSequence())
	{
		if( CanDestroyCurrentSequence() )
		{
			DestroySequence();
			ClearChipSeq(true);
		}
		else
		{
			ClearChipSeq(false);
		}
		
	}

	_mouseDown = false;
	_sequenceStarted = false;
}

bool GameField::CheckFirstChipInChain(Game::Square* sq_next, const bool hit_true)
{
	if(!hit_true)
	{
		return false; //Первые фишки должны попадать точно
	}
	if(_firstChipsInChain.empty())
	{
		if(Game::CheckMaxSeq(sq_next->address, 3))
		{
			_firstChipsInChain.push_back(sq_next->address);
			_moveWaitTimer = 0.f;
		}
		return false;
	}
	//Проверяем индекс на повтор
	for(std::list<Game::FieldAddress>::iterator it = _firstChipsInChain.begin(); it != _firstChipsInChain.end(); it++)
	{
		if(*it == sq_next->address)
		{
			return false;
		}
	}
	MyAssert(_firstChipsInChain.size() == 1 || _firstChipsInChain.size() == 2);
	//Проверяем что делать дальше. Набивать список первых фишек, сделать pop, push, или начать выделять цепочку
	//1.Проверими расстояние до фишек
	//Между последней и текущей расстояние должно быть 1
	if(_firstChipsInChain.size() != 1)
	{
		if(abs(_firstChipsInChain.back().GetCol() - sq_next->address.GetCol()) > 1 || abs(_firstChipsInChain.back().GetRow() - sq_next->address.GetRow()) > 1)
		{
			GameSettings::gamefield[_firstChipsInChain.back()]->GetChip().Deselect();
			_firstChipsInChain.pop_back();
		}	
		if(_firstChipsInChain.empty())
		{
			return false;
		}
	}
	IPoint first_index = _firstChipsInChain.front().ToPoint();
	IPoint delta = sq_next->address.ToPoint() - first_index;
	if(abs(delta.x) > 2 || abs(delta.y) > 2)
	{
		GameSettings::gamefield[_firstChipsInChain.front()]->GetChip().Deselect();
		_firstChipsInChain.pop_front();
		return false;
	}
	else if(abs(delta.x) == 2 || abs(delta.y) == 2)
	{		
		//Имеются две фишки. Курсор уже над третьей, цвет которой как у первой.
		int color = GameSettings::gamefield[_firstChipsInChain.front()]->GetChip().GetColor();
		if(delta.x == 2 && delta.y == 2)
		{
			//Диагональ - цепочка не получается в любом случае между первой и третей!
			//По крайней мере такую ситуацию, ПОКА ТОЧНО, не обрабатываем ни как.
			GameSettings::gamefield[_firstChipsInChain.front()]->GetChip().Deselect();
			_firstChipsInChain.pop_front();
			return false;
		}
		else
		{
			//Рассматриваются идентичные ситуации для вертикального и горизонтального матчей.
			//Берутся соседние фишки и смотрится подходят ли они по цвету с первой и третей.
			Game::Square *sq1, *sq2;
			if(abs(delta.x) == 2){
				sq1 = GameSettings::gamefield[Game::FieldAddress(first_index + IPoint(delta.x/2, 1))];
				sq2 = GameSettings::gamefield[Game::FieldAddress(first_index + IPoint(delta.x/2,-1))];
			} else {
				sq1 = GameSettings::gamefield[Game::FieldAddress(first_index + IPoint( 1, delta.y/2))];
				sq2 = GameSettings::gamefield[Game::FieldAddress(first_index + IPoint(-1, delta.y/2))];
			}
			if( (Game::isVisible(sq1) && sq1->GetChip().IsColor(color)) || (Game::isVisible(sq2) && sq2->GetChip().IsColor(color)))
			{						
				//Матч провести можно. Начинаем цепочку из трех фишек.
			}else{
				GameSettings::gamefield[_firstChipsInChain.front()]->GetChip().Deselect();
				_firstChipsInChain.pop_front();
				return false;
			}				
		}
	}
	if(_firstChipsInChain.empty())
	{
		return false;
	}
	Game::Square *sq_for_push = NULL;
	Game::Square *sq_first = GameSettings::gamefield[_firstChipsInChain.front()];
	if(sq_first->GetChip().GetColor() == sq_next->GetChip().GetColor() || sq_first->GetChip().IsAdapt() || sq_next->GetChip().IsAdapt() || (sq_first->GetChip().HasHang() && sq_next->GetChip().HasHang()))
	{
		//Цепочка началась с двух первых фишек.  
		sq_for_push = sq_first;					
	}
	else if(_firstChipsInChain.size() == 1)
	{
		if(Game::CheckMaxSeq(sq_next->address, 3)) {
			_firstChipsInChain.push_back(sq_next->address);
			_moveWaitTimer = 0.f;
		}
		return false;
	}
	else
	{
		Game::Square *sq_second = GameSettings::gamefield[_firstChipsInChain.back()];
		if(sq_second->GetChip().GetColor() == sq_next->GetChip().GetColor() || sq_second->GetChip().IsAdapt() || sq_next->GetChip().IsAdapt() || (sq_first->GetChip().HasHang() && sq_next->GetChip().HasHang()))
		{
			sq_for_push = sq_second;
		}
		else
		{
			GameSettings::gamefield[_firstChipsInChain.front()]->GetChip().Deselect();
			_firstChipsInChain.pop_front();
			if(Game::CheckMaxSeq(sq_next->address, 3))
			{
				_firstChipsInChain.push_back(sq_next->address);
				_moveWaitTimer = 0.f;
			}
			return false;
		}
	}
	//Определилиь с началом цепочки.
	SequenceStart(sq_for_push, 2);
	return true;
}

void GameField::MouseMove(const IPoint &mouse_pos)
{
	_lastMouseScreenPos = mouse_pos;

	if (Core::mainInput.GetMouseLeftButton())
	{
		Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
		if(index.IsValid() && Energy::field.EnergyExists(index)/* && SelectingSequence()*/)
		{
			if(_energyWaveDelay <= 0.0f )
			{
				AddEnergyWave( GameSettings::ToFieldPos(mouse_pos), "EnergyWave" );
			}
		}
	}

	if (isBlocked())
	{
		return;
	}

	for (GameFieldControllerList::iterator i = _controllers.begin(); i != _controllers.end(); i++)
	{
		if((*i)->MouseMove(mouse_pos))
		{
			return;
		}
	}

	if (GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos))
	{
		Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);
		GameSettings::underMouseIndex = index.ToPoint();

		_lastMouseFieldPos = GameSettings::ToFieldPos(mouse_pos);
		Game::Square *sq = GameSettings::gamefield[GameSettings::underMouseIndex];
		if(Game::isVisible(sq))
		{
			sq->GetChip().CheckBlick();

#ifdef ENGINE_TARGET_WIN32
			if (gameInfo.IsDevMode())
			{
				if (!EditorUtils::editor)
				{
					if (Core::mainInput.GetMouseLeftButton() && Core::mainInput.IsControlKeyDown())
					{
						// Раcчищаем клетки в игре. Нужно для гейм-дизайнеров, 
						// раccтавляющих рюшки на поле, чтобы не проходить уровнь
						Editor_InstantWallRemove(index);
						return;
					}
				}
			}
#endif
		}
	}

	Gadgets::ryushki.MouseMove(mouse_pos);
	if(!Core::mainInput.GetMouseLeftButton() || _endLevel.IsRunning())
	{
		_sequenceStarted = false;
	}else if( Match3GUI::ActCounter::GetCounter() > 0 && Match3GUI::TimeCounter::GetTime() > 0.0f)
	{
		if(!_sequenceStarted)
		{
			//Первый клик еще не обработан (бывает если начать тап с пусой клетки)
			ProcessFirstClick(mouse_pos);
		}else{
			if(SelectingSequence() && Core::mainInput.GetMouseRightButton())
			{
				_sequenceStarted = false;
				ClearChipSeq(false);
				return;
			}

			if( GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos) )
			{
				Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos - Game::ChipColor::CHIP_TAP_OFFSET);

				if (!Game::activeRect.Contains(index.ToPoint())){
					if(!_chipSeq.empty())
					{
						//Это нужно сделать чтобы была возможность водить за экраном, но чтобы при этом могли выделяться фишки рядом с тапом (умное добавление)
						ChipsChain_ProcessChip(index);
					}
					return;
				}		
				Game::Square *sq_next = GameSettings::gamefield[index];

				if( Game::isVisible(sq_next) && !sq_next->IsHardStand() && sq_next->GetChip().IsSimpleChip()
					&& (sq_next->GetChip().IsChainHighlighted() || Game::ChipColor::tutorialChainLength == 0))
				{
					IPoint field_mp = GameSettings::ToFieldPos(mouse_pos);
					IPoint chipPos = index.Get() * GameSettings::SQUARE_SIDE + IPoint(GameSettings::SQUARE_SIDE/2, GameSettings::SQUARE_SIDE/2);
					//проверяется область попадания в клетку
					bool hit_true = abs(chipPos.x - field_mp.x + 0.f) + abs(chipPos.y - field_mp.y + 0.f) < romb_square_scale;

					if(_chipSeq.empty())
					{
						//Умный и хитрый выбор начала матча CheckFirstChipInChain.
						if(!CheckFirstChipInChain(sq_next, hit_true))
						{
							//Цепочка не началась прпробуем сделать быстрый матч.
							AddressVector vec;
							int count = CheckMinPath(_realFirstChip, sq_next->address, vec, 10);
							if(count > 0 && _realFirstChipTime > 0 && _chipSeq.empty())
							{
								IPoint dist = GameSettings::underMouseIndex- _realFirstChip.ToPoint();
								if(math::max(math::abs(dist.x), math::abs(dist.y)) > 1)
								{	
									SequenceStart(GameSettings::gamefield[_realFirstChip], 10);
									return;
								} 
							}
							return;
						}
					}
					//Рассматриваем ситуацию когда не нужно точное попадание в 8-ми угольник
					if(!hit_true && index != _chipSeq.back())
					{
						FPoint dir = index.ToPoint() - _chipSeq.back().ToPoint();
						dir.Normalize();
						FPoint cross_dir = FPoint(dir.y, -dir.x)*GameSettings::SQUARE_SIDEF*0.5f;
						Game::FieldAddress index1 = GameSettings::GetMouseAddress(mouse_pos + cross_dir.Rounded() - Game::ChipColor::CHIP_TAP_OFFSET);
						Game::FieldAddress index2 = GameSettings::GetMouseAddress(mouse_pos - cross_dir.Rounded() - Game::ChipColor::CHIP_TAP_OFFSET);
						//Для случая не диагональ
						if(dir.x == 0 || dir.y == 0)
						{
							bool is_color1 = (index1 == index);
							bool is_color2 = (index2 == index);
							MyAssert(is_color1 || is_color2);
							if(!is_color1){
								Game::Square  *sq_near = GameSettings::gamefield[index1];
								is_color1 = Game::isVisible(sq_near) && sq_near->GetChip().IsColor(_chipSeqColor);
							}else{
								Game::Square  *sq_near = GameSettings::gamefield[index2];
								is_color2 = Game::isVisible(sq_near) && sq_near->GetChip().IsColor(_chipSeqColor);
							}
							if(is_color1 && is_color2)
							{
								return; //Спорная ситуация, ждем точного попадания
							}
						} else {
							//Для диагонального, если какая- либо из соседних ячеек нашего цвета, то ждем точного попадания
							Game::Square  *sq_near1 = GameSettings::gamefield[index1];
							if(Game::isVisible(sq_near1) && sq_near1->GetChip().IsColor(_chipSeqColor))
							{
								return;
							}
							Game::Square  *sq_near2 = GameSettings::gamefield[index2];
							if(Game::isVisible(sq_near2) && sq_near2->GetChip().IsColor(_chipSeqColor))
							{
								return;
							}
						}
					}
					ChipsChain_ProcessChip(index);
				} else if(!_chipSeq.empty()) {
					ChipsChain_ProcessChip(index);
				}
			}
		}
	}
}

void GameField::SequenceStart(Game::Square *sq_for_push, const int start_variation_distance)
{
	MyAssert(sq_for_push);
	
	_chipSeq.push_back(sq_for_push->address);
	_chipSeqColor = sq_for_push->GetChip().GetColor();
	//Определение цвета цепочки, если выделены одни хамелионы.
	if(sq_for_push->GetChip().IsAdapt()){
		_adaptChipsOnly = true;
		Game::ChipColor::COLOR_FOR_ADAPT = 0; //любой цвет
	}else{
		_adaptChipsOnly = false;
		Game::ChipColor::COLOR_FOR_ADAPT = _chipSeqColor;
	}
	sq_for_push->GetChip().PlaySelected(true);
	sq_for_push->setFirstHighlight(true);
	
	for(auto i:_firstChipsInChain)
	{
		GameSettings::gamefield[i]->GetChip().Deselect();
	}
	_firstChipsInChain.clear();

	sq_for_push->GetChip().Select(_chipSeq.size(), sq_for_push->address.ToPoint());
	
	OnAddChipInChain(sq_for_push->address);

	//MM::manager.PlaySample("AddChipInChain", false, 1.f,  _chipSeq.size() * 0.05f + 0.5f);
}

int GameField::CheckMinPath(const Game::FieldAddress &last_address, const Game::FieldAddress &dst, AddressVector &path, const int max_wave_iter_count)
{
	if (last_address == dst)
	{
		return 0;
	}

	// ВОЛНОВОЙ АЛГОРИТМ ПОИСКА ПУТИ

	// Вначале обнуляем вcе пути в видимом районе
	for (int x = Game::visibleRect.LeftBottom().x; x <= Game::visibleRect.RightTop().x; x++)
	{
		for (int y = Game::visibleRect.LeftBottom().y; y <= Game::visibleRect.RightTop().y; y++)
		{
			Game::Square *square = GameSettings::gamefield[x+1][y+1];
			square->minPathSource = Game::FieldAddress(x,y);
			square->minPathTime = -1;
		}
	}
	// Теперь пуcкаем волну
	Game::FieldAddress src = last_address;
	{
		Game::Square *square = GameSettings::gamefield[src];
		square->minPathSource = src;
		square->minPathTime = 0;
	}
	AddressVector oldFront, newFront;
	oldFront.push_back(src);
	int minPathTime = 0;
	int wave_iter_count = max_wave_iter_count;
	while (wave_iter_count > 0)
	{
		wave_iter_count--;
		for (AddressVectorIterator i = oldFront.begin() ; i != oldFront.end() ; ++i)
		{
			for (size_t k = 0 ; k < Gadgets::checkDirsChains.count ; ++k)
			{
				Game::FieldAddress testChip((*i).GetCol() + Gadgets::checkDirsChains.dx[k], (*i).GetRow() + Gadgets::checkDirsChains.dy[k]);
				if (!CanAddChipToSeq(testChip, last_address)){
					continue;
				}
				Game::Square *square = GameSettings::gamefield[testChip];
				if (square->minPathTime != -1){
					continue;
				}
				square->minPathTime = minPathTime + 1;
				square->minPathSource = (*i);
				newFront.push_back(testChip);
			}
		}
		if (newFront.size() == 0)
			// Выходим из цикла, вcе клетки обработаны
			break;
		// Смотрим вcе клетки, вдруг в них еcть нужная нам, а значит путь найден!
		for (AddressVectorIterator i = newFront.begin() ; i != newFront.end() ; ++i)
		{
			if ((*i) == dst)
			{
				// Ура нашли, cоcтавляем обратный путь
				Game::FieldAddress j = dst;
				while (j != src)
				{
					path.push_back(j);
					j = GameSettings::gamefield[j]->minPathSource;
				}
				return minPathTime;
			}
		}
		oldFront = newFront;
		newFront.clear();
		minPathTime++;
	}
	// Не нашли
	return -1;
}

// Проверяем, что можно cделать c фишкой c координатами (i, j) и цепочкой.
// Еcли в результате цепочка была изменена, то результат будет true,
// еcли цепочка не была изменена, то результат будет false
bool GameField::ChipsChain_ProcessChip(const Game::FieldAddress& address)
{
	bool aloneChip = true;

	// Проверяем, может ли ячейка под мышкой потенциально быть в цепочке?
	bool allowSequence = Game::isNormalChip(address) && Game::isStandbyChip(address);
	if (allowSequence)
	{
		Assert(_chipSeq.size() > 0);
		// Мы над поcледней ячейкой, выходим...
		if (_chipSeq.back() == address){
			return false;
		}

		Game::Square *sq_first = GameSettings::gamefield[_chipSeq.front()];
		Game::Square *sq_mouse = GameSettings::gamefield[address];

		// в туториале позволяем выделять только то, что сказано
		if( Game::ChipColor::tutorialChainLength > 0 && !sq_mouse->GetChip().IsChainHighlighted() )
		{
			//return false; //возвращать сразу не надо, проверим еще умным алгоритмом проверки соседних клеток
		}else{
			if(!sq_mouse->GetChip().IsAdapt() && _adaptChipsOnly)
			{
				_adaptChipsOnly = false;
				Game::ChipColor::COLOR_FOR_ADAPT = sq_mouse->GetChip().GetColor(); //С цветом последовательности определились
			}
			_chipSeqColor = sq_first->GetChip().GetColor();

			// Получаем цвет фишки под курcором
			int colorMouse = sq_mouse->GetChip().GetColor();

			aloneChip = !sq_mouse->IsColor(_chipSeqColor);

			// Нужно ли укоротить цепочку?
			size_t count = _chipSeq.size();
			for (size_t k = 0; k < count; k++)
			{
				if (_chipSeq[k] != address)
				{
					continue;
				}
				 //Укорачивать можно только на две фишки не более. Как в Джелли Сплэш. Т.е. проверяем только две предпоследние фишки.
				if(k == count - 3 || k == count - 2)
				{
					ResizeChipSeq(k + 1, false);

					_chipSeqColor = GetChainColor(_chipSeq);
					Game::ChipColor::COLOR_FOR_ADAPT = _chipSeqColor;			
				}
				return true;
			}

			int dx = address.GetCol() - _chipSeq.back().GetCol();
			int dy = address.GetRow() - _chipSeq.back().GetRow();
		
			bool distOK = (abs(dx) + abs(dy) == 1) || (abs(dx) == 1 && abs(dy) == 1);

			bool bonusMatched = (_chipSeq.size() == 1)
				&& sq_first->GetChip().HasHang() && sq_mouse->GetChip().HasHang()
				&& (Gadgets::levelSettings.getString("AllowBonusMatch") == "true");

			// Если сматчили два бонуса разных цветов, продолжать цепочку нельзя
			if(IsBonusComboSequence(_chipSeq)) {
				Game::Square *sq1 = GameSettings::gamefield[_chipSeq.front()];
				Game::Square *sq2 = GameSettings::gamefield[_chipSeq.back()];
				if(sq1->GetChip().GetColor() != sq2->GetChip().GetColor()) {
					return false;
				}
			}

			// Добавим, еcли правильное раccтояние и цвет
			if (distOK && ((colorMouse == _chipSeqColor) || bonusMatched))
			{
				_chipSeq.push_back(address);

				GameSettings::gamefield[address]->GetChip().PlaySelected(true);
				if (_chipSeq.size() > 1)
				{
					GameSettings::gamefield[_chipSeq[0]]->setFirstHighlight(false);
					Game::Square *sq = GameSettings::gamefield[_chipSeq.back()];
					sq->GetChip().Select(_chipSeq.size(), sq->address.ToPoint(), true, _chipSeq[_chipSeq.size()-2].ToPoint());
				}

				//MM::manager.PlaySample("AddChipInChain", false, 1.f, _chipSeq.size() * 0.05f + 0.5f);
				OnAddChipInChain(address);
				return true;
			}
		}
	}

	//Далее идет проверка окружающих клеток на возможность проведения через них цепочку

	const int ox[9] = {0, -1, 1, 0,  0, 1, -1, -1,  1};
	const int oy[9] = {0,  0, 0, 1, -1, 1,  1, -1, -1};
	AddressVector minPath;
	bool minPathFinded = false;
	for (int k = 0 ; k < 9 ; ++k)
	{
		if (k > 0 && !aloneChip)
			break;
		Game::FieldAddress testChip(address.GetCol() + oy[k], address.GetRow() + ox[k]);
		if ((_chipSeq.back() != testChip) && !CanAddChipToSeq(testChip, _chipSeq.back()))
		{
			continue;
		}
		AddressVector testPath;
		if (CheckMinPath(_chipSeq.back(), testChip, testPath, (_realFirstChipTime > 0 ? 10 : 2)) != -1)
		{
			int dx = abs(testChip.GetCol() - _chipSeq.back().GetCol());
			int dy = abs(testChip.GetRow() - _chipSeq.back().GetRow());
			if ((dx == 1) && (dy == 1) && (testPath.size() > 2))
				continue;
			
			if (k == 0)
			{
				minPath = testPath;
				minPathFinded = true;
				break;
			}
			else if (!minPathFinded)
			{
				minPath = testPath;
				minPathFinded = true;
			}
			else if (minPath.size() > testPath.size())
			{
				minPath = testPath;
			}
		}
		else if (k == 0)
		{
			// фишка одна того же цвета, и надо проверить радиуc вокруг
			aloneChip = true;
		}
	}
	if (minPathFinded)
	{
		for (AddressVector::reverse_iterator i = minPath.rbegin() ; i != minPath.rend() ; ++i)
		{
			_chipSeq.push_back(*i);
			Game::Square *sq = GameSettings::gamefield[*i];
			if(_chipSeq.size() > 1)
			{
				sq->GetChip().Select(_chipSeq.size(), sq->address.ToPoint(), true, _chipSeq[_chipSeq.size()-2].ToPoint());
			}else{
				sq->GetChip().Select(_chipSeq.size(), sq->address.ToPoint());
			}
		}
		if (minPath.size() > 0)
		{
			OnAddChipInChain(_chipSeq.back());
			//MM::manager.PlaySample("AddChipInChain", false, 1.f, _chipSeq.size() * 0.05f + 0.5f);
		}
		return minPath.size() > 0;
	}
	//От поиска с перестраиванием(возвратом) уже взятых фишек тоже пока отказались.
	//// Еcли минимального пути не было обнаружено, то возможно мы захотели поменять cаму цепочку c разрывом cтарой
	//// Но для этого надо проверить вcе пути в радиуcе текущей до первой фишки в цепочке
	//if (allowSequence &&
	//	GameSettings::gamefield[address]->IsColor(colorLast) &&
	//	(std::find(_chipSeq.begin(), _chipSeq.end(), address) == _chipSeq.end()))
	//{
	//	AddressVector _chipSeqOld = _chipSeq;
	//	for (size_t i = 0 ; i < _chipSeqOld.size() - 1 ; ++i)
	//	{
	//		// Убираем поcледнюю фишку в цепочке
	//		_chipSeq.pop_back();
	//		// Ищем минимальный путь до текущей клетки
	//		AddressVector testPath;
	//		if (CheckMinPath(address, testPath) != -1)
	//		{
	//			int dx = abs(address.GetCol() - _chipSeq.back().GetCol());
	//			int dy = abs(address.GetRow() - _chipSeq.back().GetRow());
	//			if ((dx == 1) && (dy == 1) && (testPath.size() > 2))
	//				continue;
	//			
	//			for (AddressVector::reverse_iterator i = testPath.rbegin() ; i != testPath.rend() ; ++i)
	//			{
	//				_chipSeq.push_back(*i);
	//			}

	//			OnAddChipInChain(address);
	//			MM::manager.PlaySample("AddChipInChain", false, 1.f, _chipSeq.size() * 0.05f + 0.5f);

	//			return true;
	//		}
	//	}
	//	_chipSeq = _chipSeqOld;
	//}
	return false;
}

bool GameField::CanAddChipToSeq(const Game::FieldAddress &address, const Game::FieldAddress &last_address)
{
	// Еcли не в площадке, то продолжаем оcмотр
	if (!Game::activeRect.Contains(address.ToPoint()))
	{
		return false;
	}
	// Еcли это фишка, а не какой-нибудь другой объект
	bool allowAdd = Game::isNormalChip(address) && Game::isStandbyChip(address);
	if (!allowAdd){
		return false;
}
	// Наконец фишка :)
	Game::Square *addressSquare = GameSettings::gamefield[address];
	// cмотрим цвет, еcли cразу не cовпадает, то продолжаем оcмотр
	Assert(Game::isVisible(last_address));
	int addressColor = addressSquare->GetChip().GetColor();
	if ( !GameSettings::gamefield[last_address]->GetChip().IsColor(addressColor))
	{
		return false;
	}
	if( Game::ChipColor::tutorialChainLength > 0 && !addressSquare->GetChip().IsChainHighlighted() )
	{
		return false;
	}
	// Еcли попали в фишку из цепочки, то тоже не надо добавлять ничего
	if (std::find(_chipSeq.begin(), _chipSeq.end(), address) != _chipSeq.end())
	{
		return false;
	}
	return true;
}

void GameField::CalcSequenceEffects()
{
	_chipSeqAffectedVisual = _chipSeqAffected;
	_chipSeqAffected.clear();	
	if(CanHighlightCurrentSequence())
	{
		// не очищаем информацию о бонусах для коротких цепочек, т.к. она нам нужна для запуска
		// бонусов уже после разрушения цепочки, т.е. по сути здесь будет информация верная не
		// для текущей (возможно короткой или нулевой), а для последней правильной (длиной >3) цепочки
		_bonusCascade.clear();

		const bool splitSequence = (Gadgets::levelSettings.getString("SplitSequence") == "true");
		if( splitSequence )
		{
			int count = (int)_chipSeq.size();
			int L[3];
			size_t last_s = 0;
			for(size_t i = 0; i < 3; i++)
			{
				int seq_size = Gadgets::levelSettings.getInt(std::string("Seq") + utils::lexical_cast(i));
				if(seq_size <= count)
				{
					L[i] = seq_size;
					last_s = i;
				}else{
					L[i] = -1;
				}
			}
			L[last_s] = count;
			AddressVector::iterator it_start = _chipSeq.begin();
			for(size_t i = 0; i <= last_s; i++)
			{
				AddressVector::iterator it_end = _chipSeq.begin() + L[i] - 1;
				MyAssert(it_start < it_end);
				AddressVector seq_next(it_start, it_end + 1);
				CalcSubSequenceEffects(seq_next, _chipSeqAffected);
				it_start = it_end;
			}
		}
		else
		{
			CalcSubSequenceEffects(_chipSeq, _chipSeqAffected);
		}
	}

	//_showAffectedZone = _chipSeqAffected.size() > 0;
 
	int colors[32];
	for(int i = 0; i < 32; ++i)
		colors[i] = 0;
	for(std::map<Game::FieldAddress, bool>::const_iterator itr = _chipSeqAffected.begin(); itr != _chipSeqAffected.end(); ++itr)
	{
		Game::Square *sq = GameSettings::gamefield[itr->first];
		if( sq->GetChip().IsSimpleChip() ){
			++colors[sq->GetChip().GetColor()];
		}
	}
	for(int i = 0; i < 32; ++i) {
		Game::Orders::SelectSequence( Game::Order::Objective(i), colors[i]);
	}

	std::string tooltip = Game::Orders::GetTooltip();
	if( !tooltip.empty() )
	{
		_orderText->SetSource(tooltip);
		_orderTooltip.Show(_orderText, 0.f, 100000.f);
	}
	else
	{
		_orderTooltip.HideSmoothly();
	}
	if(_chipSeqAffected.size() > 0)
	{
		_chipSeqAffectedVisual = _chipSeqAffected;
	}
}

void GameField::CalcSubSequenceEffects(AddressVector &seq, std::map<Game::FieldAddress, bool> &result)
{
	Game::Hang bonus = getCurrentSequenceBonus(seq);
	bool combineBonusOnChain = (Gadgets::levelSettings.getString("CombineBonusChain") == "true") || IsBonusComboSequence(seq);

	for(AddressVector::iterator itr = seq.begin(); itr != seq.end(); ++itr)
	{
		Game::Square *sq = GameSettings::gamefield[*itr];
		if( itr == seq.end()-1 ){
			// последняя фишка в цепочке, добавить зону поражения суммарного бонуса с цепочки
			CalcBonusEffects(sq->address, bonus, result);
		} else {
			if( combineBonusOnChain ){
				// если мы суммируем бонусы в цепочке, то бонус взрывается только на ее конце
				result.insert(std::make_pair(sq->address, false));
			} else {
				// а если нет, то взрывается бонус навешенный на каждой фишке
				CalcBonusEffects(sq->address, sq->GetChip().GetHang(), result);
			}
		}
	}
}

bool GameField::CheckSquareOnBonusAndCalc(Game::Square *sq, std::map<Game::FieldAddress, bool> &result, Game::Hang *parent_hang, Game::FieldAddress parent_hang_cell)
{
	if(sq == 0)
	{
		return false;
	}
	const bool combineBonuses = (Gadgets::levelSettings.getString("CombineBonusCascade") == "true");
	if( sq->GetChip().IsExist() && sq->GetChip().HasHang())
	{
		Game::Hang b = sq->GetChip().GetHang();
		if( combineBonuses && parent_hang)
		{
			b.Add(*parent_hang, false, parent_hang_cell.ToPoint() - sq->address.ToPoint());
		}
		CalcBonusEffects(sq->address, b, result);
		return true;
	}
	else
	{
		std::vector<BombField::HardPtr> bombs;
		Gadgets::bombFields.GetBombInPoint(sq->address.ToPoint(), bombs);
		if(!bombs.empty())
		{
			//Тут есть бомба. Обсчитываем ее.
			//for(size_t i = 0; i < bombs.size(); i++)
			size_t i = 0; //Одной бомбы хватит наверняка. Иначе все равно остальные проигнорятся в CalcBonusEffects.
			{
				Game::Hang new_hang;
				new_hang.MakeBomb(bombs[i]->bangRadius, 3);
				CalcBonusEffects(sq->address, new_hang, result);
			}
			return true;
		}
	}
	return false;
}

void GameField::CalcBonusEffects(Game::FieldAddress cell, Game::Hang bonus, std::map<Game::FieldAddress, bool> &result)
{
	// сначала добавим клетку, из которой взрывается бонус
	// если она уже была добавлена, значит этот бонус уже был обработан и нужно выйти
	std::map<Game::FieldAddress, bool>::iterator cell_itr = result.find(cell);
	if(cell_itr == result.end())
	{
		result.insert(std::make_pair(cell, !bonus.IsEmpty()));
		Game::AffectedArea cells;
		bonus.GetAffectedChips(cell, cells, 0.0f);

		// для того чтобы подсвеченная зона поражения совпадала с тем, что реально сдетонирует потом, некоторые
		// бонусы важно добавлять в каскад после вызова GetAffectedChips, где рассчитывается их зона поражения
		if(!bonus.IsEmpty())
			_bonusCascade.push_back( std::make_pair(cell, bonus) );

		for(Game::AffectedArea::iterator itr = cells.begin(); itr != cells.end(); ++itr)
		{
			Game::Square *sq = GameSettings::gamefield[itr->first];
			if(!CheckSquareOnBonusAndCalc(sq, result, &bonus, cell))
			{
				//result.insert(std::make_pair(itr->first, true));
				result[itr->first] = true;
			}
		}
	}
	else
	{
		cell_itr->second = cell_itr->second || !bonus.IsEmpty();
	}
}

void GameField::OnSequenceDestroyed()
{
	if(_bonusCascade.empty()) {
		EndMove();
	} else {
		Game::AddController(new CombinedBonus(_bonusCascade, this, true, CombinedBonus::CHAIN));
		_bonusCascade.clear();
	}
	CalcSequenceEffects();

	//_showAffectedZone = false;
	//_chipSeqAffected.clear(
	//_orderTooltip.HideSmoothly();
	//for(int i = 0; i < 32; ++i) {
	//	Game::Orders::SelectSequence( Game::Order::Objective(i), 0);
	//}
}

bool GameField::CanDestroyCurrentSequence() const
{
	return ((_chipSeq.size() > 2 && !_adaptChipsOnly) || IsBonusComboSequence(_chipSeq)) && (_chipSeq.size() >= Game::ChipColor::tutorialChainLength);
}

bool GameField::CanHighlightCurrentSequence() const
{
	return _chipSeq.size() > 1;
}

bool GameField::IsBonusComboSequence(const AddressVector &seq) const
{
	if((seq.size() == 2) && (Gadgets::levelSettings.getString("AllowBonusMatch") == "true")) {
		Game::Square *sq1 = GameSettings::gamefield[seq.front()];
		Game::Square *sq2 = GameSettings::gamefield[seq.back()];
		return sq1->GetChip().HasHang() && sq2->GetChip().HasHang();
	} else {
		return false;
	}
}

void GameField::DestroySequence()
{
	Assert( CanDestroyCurrentSequence() );
	_realFirstChip.SetCol(-1);
	_realFirstChip.SetRow(-1);
	
	int len = (int)_chipSeq.size();

	//прибавляем число возможных ходов (пока не испортилось)
	Game::totalPossibleMoves += _fuuuTester.CountPossibleMoves();

	levelsInfo.levelStarted = true;
	_chipSeqCopy = _chipSeq;

	GameSettings::need_inc_wall_squares = 1;

	GameSettings::need_inc_growing_wood = 1;

	//Перед новым матчем, разлачиваем все блокировки клеток
	for(int x = Game::visibleRect.LeftBottom().x; x <= Game::visibleRect.RightTop().x; x++)
	{
		for(int y = Game::visibleRect.LeftBottom().y; y <= Game::visibleRect.RightTop().y; y++)
		{
			Game::Square *sq = GameSettings::gamefield[IPoint(x, y)];
			if(!Game::isBuffer(sq))
			{
				sq->SetBusyNear(0);
				sq->SetBusyCell(0);
			}
		}	
	}

	if( Gadgets::levelSettings.getString("FILL_ItemType") == "Fires" )
	{
		// Если повезет, даем бонусный огонек в зачет накапливаемого бонуса
		int seqSize = (int)_chipSeq.size();
		if( seqSize > 2 && seqSize < Gadgets::levelSettings.getInt("LSeq0"))
		{
			int chance = Gadgets::levelSettings.getInt(std::string("FireChance") + utils::lexical_cast(std::min(seqSize, 8)));
			if( math::random(0, 99) < chance )
			{
				FPoint pos = FPoint(_chipSeq.back().GetCol() + 0.5f, _chipSeq.back().GetRow() + 0.5f) * GameSettings::SQUARE_SIDEF;
				Game::AddController(new FlyFillBonusEffect(this, pos));
			}
		}

		if(_seqFires) {
			_seqFires->FlyToFillBonus();
			_seqFires = NULL;
		}
	}
	else
	{
		Match3GUI::FillBonus::Get().ChangeCounter( gameInfo.getLocalInt("FILL_bonus_current_s_count") );
	}

	_timeFromLastMove = 0.f;
//	MM::manager.PlaySample("ClearChain");

	ResetTimersAfterAction();

	// Пометим все клетки, которые будут уничтожены этим ходом
	for(std::map<Game::FieldAddress, bool>::iterator itr = _chipSeqAffected.begin(); itr != _chipSeqAffected.end(); ++itr) {
		GameSettings::gamefield[itr->first]->MarkToBeDestroyed();
	}


	const bool splitSequence = (Gadgets::levelSettings.getString("SplitSequence") == "true");
	if(splitSequence)
	{
		// В таком случае длинные цепочки разбиваем на несколько, и соответственно добавляем несколько бонусов
		int count = (int)_chipSeq.size();
		for(size_t i = 0; i < 3; i++)
		{
			int seq_size = Gadgets::levelSettings.getInt(std::string("Seq") + utils::lexical_cast(i));
			AddressVector::iterator it_start = _chipSeq.begin();
			if(seq_size <= count)
			{
				if(i < 2)
				{
					//Последний бонус навешиваем на последнюю фишку в последовательности
					int seq_size_next = Gadgets::levelSettings.getInt(std::string("Seq") + utils::lexical_cast(i+1));
					if(seq_size_next > count)
					{
						seq_size = count;
					}
				}else
				{
					//Последняя последовательность забирает все
					seq_size = count;
				}

				AddressVector::iterator it_end = _chipSeq.begin() + seq_size - 1;
				MyAssert(it_start < it_end);
				AddressVector seq_next(it_start, it_end + 1);
				SequenceDestroyer *sd = new SequenceDestroyer(seq_next, this);
				Game::AddController(sd);
				it_start = it_end;
			}
		}
	}
	else
	{
		// а не диагональные уничтожаем одним куском, как обычно
		SequenceDestroyer *sd = new SequenceDestroyer(_chipSeq, this);
		Game::AddController(sd);
	}

	Core::messageManager.putMessage(Message("DestroyChain", len));

	if (len > 3 && len < 6) {
		//MM::manager.PlaySample("Cheers1");
	} else if (len > 5) {
		//MM::manager.PlaySample("Cheers2");
	}

	OnStartMove();
}

void GameField::HangBonus(const std::string& bonusChip, const Game::Hang& hang, size_t count_bonuses)
{
	if( bonusChip == "screen" ) // Здесь бонусы летят через поле
	{
		// при данном варианте бонусы вылетают сразу при разрушении фишки (реализовано в SequenceDestroyer)

		//for(size_t i = 0; i < count_bonuses; ++i)
		//{
		//	int chipNum = (i < 3) ? Gadgets::levelSettings.getInt("Seq" + utils::lexical_cast(i)) : _chipSeqCopy.size();
		//	FPoint pos = (FPoint(0.5f, 0.5f) + _chipSeqCopy[chipNum-1].ToPoint()) * GameSettings::SQUARE_SIDEF;
		//	Game::FieldAddress fa(-1,-1);
		//	if(!_tutorialHangBonusSquares.empty()) {
		//		fa = _tutorialHangBonusSquares.front();
		//		_tutorialHangBonusSquares.pop_front();
		//	}
		//	Game::AddController(new Game::Spirit(GameSettings::ToScreenPos(pos), bonusChip, hang, _chipSeqCopy, fa));
		//}
	}
	else // Здесь просто вешаются по соседству
	{
		std::vector<Game::Square*> bonusSquares;
	
		if(_tutorialHangBonusSquares.empty()) {
			bonusSquares = Game::ChooseSquaresForBonus(bonusChip, count_bonuses, _chipSeqCopy);
		} else {
			// мы в туториале, нужно "случайно" повесить бонус на заданную клетку
			size_t count = std::min(count_bonuses, _tutorialHangBonusSquares.size());
			for(size_t i = 0; i < count; ++i) {
				Game::FieldAddress fa = _tutorialHangBonusSquares.front();
				bonusSquares.push_back( GameSettings::gamefield[fa] );
				_tutorialHangBonusSquares.pop_front();
			}
		}

		for(size_t i = 0; i < bonusSquares.size(); ++i)
		{
			bool bonusOnChip = (bonusChip == "chip" || bonusChip == "up chip" || bonusChip == "next");
			if( bonusOnChip && bonusSquares[i]->GetChip().IsExist() ) {
				// вешаем бонус именно на заданную фишку до того, как она начнет падать и перейдет в собственность к другой клетке
				bonusSquares[i]->GetChip().SetHang(hang);
			} else {
				bonusSquares[i]->SetHangForChip(hang);
			}
		}
	}
}

// Определяем первый валидный цвет в цепочке (пропуcкаем проcтого хамелеона)
int GameField::GetChainColor(const AddressVector &list, int offset, bool forward)
{
	int count = (int) list.size();
	if (count == 0) {
		return -1;
	}
	if (forward) {
		int start = offset;
		int end = count;
		if (start >= end) {
			return -1;
		}
		return GameSettings::gamefield[list[start]] -> GetChip().GetColor();
	} else {
		int start = count - 1 - offset;
		int end = 0;
		if (start < end) {
			return -1;
		}
		return GameSettings::gamefield[list[start]] -> GetChip().GetColor();
	}
	Assert(false);
	return -1;
}

void GameField::OpenEnergySource()
{
	Gadgets::square_new_info.EnergySource_Start();
	_sourceOpened = true;
	std::vector<IPoint> vec;
	Gadgets::square_new_info.EnergySource_Get(vec);
	for(size_t i = 0; i < vec.size(); i++)
	{
		Game::Square *sq = GameSettings::gamefield[vec[i]];
		sq->SetWall(0);

		FPoint pos = sq->GetCellPos() + GameSettings::CELL_HALF;

		// эффект появления
		ParticleEffectPtr eff = Game::AddEffect(GameField::Get()->_effUnderChipsCont, "EnergyBegin");
		eff->SetPos(pos);
		eff->Reset();

		// зацикленный эффект в клетках источниках
		eff = Game::AddEffect(GameField::Get()->_effTopCont, "EnergyBegin2");
		eff->SetPos(pos);
		eff->Reset();
		_energyBeginEff.push_back(eff);

		MM::manager.PlaySample("EnergyBegin");

		ResumeEnergy(1.2f);
	}
	Gadgets::freeFrontDetector.Update();
	
	if (_boostBeforeStart.empty()) {
		AcceptMessage(Message("BoostsBeforeStartFinished"));
	} else {
		Core::guiManager.getLayer("BoostersTransition")->getWidget("BoostersTransitionWidget")->AcceptMessage(Message("StartRuning"));
	}
	
	Core::LuaCallVoidFunction("OnOpenEnergySource");
}

void GameField::StopEnergySourceEffects()
{
	for(ParticleEffectPtr eff : _energyBeginEff) {
		eff->Finish();
	}
	_energyBeginEff.clear();
}

void GameField::FillLevel()
{
	Match3::SetRect(_fieldLeft, _fieldRight, _fieldBottom, _fieldTop);
	Match3::FillLevel();

	_fieldFilling = true;
	BlockField();

	if( Gadgets::levelSettings.getString("StartFill") == "true" )
	{
		IRect rect = Game::visibleRect;
		IRect rect2 = Game::targetRect;

		int x1 = std::min(rect.x, rect2.x);
		int x2 = std::max(rect.x+rect.width, rect2.x+rect2.width);
		int y1 = std::min(rect.y, rect2.y);
		int y2 = std::max(rect.y+rect.height, rect2.y+rect2.height);

		rect = IRect(x1, y1, x2-x1, y2-y1);

		Match3::SetRect(rect.x, rect.x + rect.width - 1, rect.y, rect.y + rect.height - 1);
		Match3::RefillRect();
	}

	// Вот этот цикл проходит по вcему полю и дополнительно 
	// рандомизирует цвета фишек подо льдом так, чтобы фишка подо 
	// льдом и фишка cправа или cверху не оказалиcь одинаковыми

	std::map<int, bool> colors_full, colors_current;
	for(int i = 0; i < Gadgets::levelColors.GetCount();i++)
	{
		colors_full[Gadgets::levelColors[i]] = false;
	}
	int clr;

	for (int j = GameSettings::fieldHeight - 2; j >= 0; j--)
	{
		for (int i = GameSettings::fieldWidth - 2;  i >= 0; i--)
		{
			Game::FieldAddress index(i, j);

			if(!index.IsValid())	{
				continue;
			}

			Game::Square *s = GameSettings::gamefield[index];

			if(s->IsCanChainsOnStart()){
				continue;
			}
			
			// Хамелеоны cейчаc cтавятcя вручную...
			if (s->GetChip().IsChameleon())	{
				continue;
			}

			colors_current = colors_full;

			for(size_t k = 0; k < Gadgets::checkDirsChains.count/2; k++)
			{
				Game::FieldAddress a = index + Game::FieldAddress(Gadgets::checkDirsChains[k]);
				if(Game::isNormalChip(a))
				{
					clr = GameSettings::gamefield[a]->GetChip().GetColor();
					if(colors_current.find(clr) != colors_current.end())
					{
						colors_current.erase(clr);
					}
				}
			}

			if(colors_current.size() == colors_full.size())
			{  //Совпадений нет. Незачем менять
				continue;
			}

			if(!colors_current.empty())
			{
				// Если остались цвета, то перекрашиваем в рандом из оставшихся
				std::map<int, bool>::iterator it_next_color = colors_current.begin();
				size_t rnd_idx = math::random(0u, colors_current.size()-1);
				while(rnd_idx > 0)
				{
					rnd_idx--;
					it_next_color++;
				}
				s->GetChip().SetColor(it_next_color->first);
			}
		}
	}

	FillChameleonVector();
	FindBestChameleonColors();
}

bool GameField::LoadLevelSettings()
{
	std::string level_name_full = "Levels/" + levelsInfo.GetLevelName() + ".xml";
	if(!File::Exists(level_name_full)){
		MyAssert(false);
		return false;
	}
	Xml::RapidXmlDocument rapidLevelDoc(level_name_full);
	Gadgets::LoadLevelSettings(rapidLevelDoc.first_node());
    return true;
}

bool GameField::LoadLevelRapid(const std::string &levelName, bool loadResources)
{
	AcceptMessage(Message("ClearLevel"));

	EditorUtils::lastLoadedLevel = levelName;

	std::string level_name_full = "Levels/" + levelName + ".xml";
	Log::Info("Loading level: \"" + level_name_full + "\"");
	if(!File::Exists(level_name_full)){
		MyAssert(false);
		Log::Error("GAMEFIELD:: Can not load level '" + level_name_full + "'");
		return false;
	}
	Xml::RapidXmlDocument rapidLevelDoc(level_name_full);
	rapidxml::xml_node<>* levelRoot = rapidLevelDoc.first_node();
     
	if (!levelRoot)
	{
		Log::Error("GAMEFIELD:: Can not load level '" + levelName + "'");
		return false;
	}
	else
	{
		_level = gameInfo.getLocalInt("current_level");

		Tutorial::luaTutorial.Init( Xml::GetStringAttributeOrDef(levelRoot, "script", "") );

		Gadgets::levelColors.Load(levelRoot);

		rapidxml::xml_node<> *elem;

		IPoint posLevel = IPoint(0, 0);
		if (levelRoot->first_node("PosLevel"))
		{
			posLevel = IPoint(levelRoot->first_node("PosLevel"));
		}

		elem = levelRoot->last_node("Line"); //last_node - считываем наоборот
	
		GameSettings::ClearSquares();		


		//Gadgets::snapGadgets.Clear();
		
		int lineIndex = 0;

		size_t rIndex = 0;
		while (elem)
		{
			rapidxml::xml_node<>* lineXml =  elem->first_node();
			char *line = lineXml->value();
			int lineWidth = static_cast <int> (strlen(line));

			for (int charIndex = 0; charIndex < lineWidth; charIndex++)
			{
				Game::Square* sq;
				Game::FieldAddress fa(charIndex + posLevel.x, lineIndex + posLevel.y);
				
				if( line[charIndex] == '.') {
					// дефолтное значение, куда-то записывать такую клетку нет необходимости
					// sq = Game::bufSquare;
				} else if( line[charIndex] == ',') {
					sq = Game::bufSquareNoChip;
					GameSettings::gamefield[fa] = sq;
				} else if( line[charIndex] == '`') {
					sq = Game::bufSquareNoLicorice;
					GameSettings::gamefield[fa] = sq;
				} else if( line[charIndex] == '^') {
					sq = Game::bufSquareNoTimeBomb;
					GameSettings::gamefield[fa] = sq;
				} else if( line[charIndex] == '!') {
					sq = Game::bufSquareNoBomb;
					GameSettings::gamefield[fa] = sq;
				} else {
					sq = new Game::Square(fa);

					if( line[charIndex] == 'e')
					{
						//Устаревший метод задания источника энергии. Исправлен для преемственности.
						IPoint pos = IPoint(charIndex + posLevel.x, lineIndex + posLevel.y);
						rapidxml::xml_node<>* squares_info = levelRoot->first_node("NewSquareInfo");
						if(!squares_info)
						{
							squares_info = Xml::NewNode(levelRoot, "NewSquareInfo");
						}
						rapidxml::xml_node<>*xml_cell = Xml::NewNode(squares_info, "square");
						Xml::SetStringAttribute(xml_cell, "i", utils::lexical_cast<>(pos.x));
						Xml::SetStringAttribute(xml_cell, "j", utils::lexical_cast<>(pos.y));
						Xml::SetIntAttribute(xml_cell, "en_source", 1);
					}

					sq->Deserialize(line[charIndex]);

					if ( sq->GetWall() >= 0)
					{
						GameSettings::squares.push_back(sq);
					}
				}
			}
			elem = elem->previous_sibling("Line"); //Предыдущий т.к. считываем с последней
			
			lineIndex += 1;
		}

		GameSettings::fieldWidth = 0;
		GameSettings::fieldHeight = 0;

		// Приcоединяем вcе клетки к полю...
		for (size_t i = 0; i < GameSettings::squares.size(); i++)
		{
			Game::Square *s = GameSettings::squares[i];

			s->GetChip().SetPos(FPoint(0.0f, 0.0f));

			GameSettings::gamefield[s->address] = s;
			GameSettings::recfield[s->address] = true;

			// Определяем правую и верхнюю границу уровня...
			// TODO: далее ОЧЕНЬ ПОДОЗРИТЕЛЬНЫЙ код
			if (s -> address.GetCol() + 2 > GameSettings::fieldWidth)
			{
				GameSettings::fieldWidth = s -> address.GetCol() + 2;
			}
			if (s -> address.GetRow() + 2 > GameSettings::fieldHeight)
			{
				GameSettings::fieldHeight = s -> address.GetRow() + 2;
			}
		}

		Gadgets::LoadWallTypes(levelRoot);
		Gadgets::LoadLevel(levelRoot);

		FieldStyles::LoadLevel(levelRoot);
		if(loadResources)
		{
			Gadgets::ryushki.LoadRyushki(levelRoot);
			ApplyFieldStyle();
		}

		Match3GUI::FillBonus::Get().SetCounter(0);
		gameInfo.setLocalInt("FILL_bonus_current_s_count", 0);
	}
	
	//Game::UpdateVisibleRects();
	Gadgets::InitProcessSettings();
	ComputeSquareDist();
	ComputeFieldLimits();
	levelBorder.Clear();
	//levelBorder.Calculate();

	std::string levelObj = Gadgets::levelSettings.getString("LevelObjective", "receivers");
	if( levelObj == "receivers" ) {
		_levelObjective = LevelObjective::RECEIVERS;
	} else if( levelObj == "diamonds" ) {
		_levelObjective = LevelObjective::DIAMONDS;
	} else if( levelObj == "bears" ) {
		_levelObjective = LevelObjective::BEARS;
	} else if( levelObj == "score" ) {
		_levelObjective = LevelObjective::SCORE;
	} else if( levelObj == "energy" ) {
		_levelObjective = LevelObjective::ENERGY;
	}


	if(EditorUtils::editor)
	{
		//Нужно чтобы в редакторе поле было на экране
		std::vector<IPoint> vec;
		Gadgets::square_new_info.EnergySource_Get(vec);
		if(!vec.empty())
		{
			GameSettings::fieldX = vec.front().x*GameSettings::SQUARE_SIDE - GameSettings::FIELD_SCREEN_CONST.width/2 + 0.f;
			GameSettings::fieldY = vec.front().y*GameSettings::SQUARE_SIDE - GameSettings::FIELD_SCREEN_CONST.height/2 + 0.f;
			//OptimizationReason type = REASON_NO_OPTIMIZATION;
			//OptimizeFieldPos(GameSettings::fieldX, GameSettings::fieldY, type);
		}else{
			Assert(Gadgets::levelSettings.getString("LevelObjective", "receivers") != "receivers");
			GameSettings::fieldX = (_fieldLeft - GameSettings::COLUMNS_COUNT/2)*GameSettings::SQUARE_SIDEF + GameSettings::FIELD_SCREEN_CENTER.x;
			GameSettings::fieldY = (_fieldBottom - GameSettings::ROWS_COUNT/2)*GameSettings::SQUARE_SIDEF + GameSettings::FIELD_SCREEN_CENTER.y;
		}
	}

	return true;
}

void SortAttributes(Xml::TiXmlElement *root)
{
	if (root) {
		for (Xml::TiXmlNode *node = root->FirstChild(); node; node = node->NextSibling()) {
			Xml::TiXmlElement *element = node->ToElement();

			if (element) 
			{
				// в мапе само отсортирует
				std::map<std::string, std::string> attrs;

				//берем атрибуты
				for(Xml::TiXmlAttribute *attr = element->FirstAttribute(); attr; attr = attr->Next() )
				{
					attrs.insert(std::make_pair(attr->Name(), attr->Value()));
					//if ( node->name == name )
					//	return node;
				}

				//удаляем
				for (auto attrPair:attrs)
				{
					element->RemoveAttribute(attrPair.first);
				}

				// заполняем отсортированными
				for (auto attrPair:attrs)
				{
					element->SetAttribute(attrPair.first, attrPair.second);
				}

				//рекурсивно обрабатываем
				SortAttributes(element);
			}
        }
	}
}

void GameField::SaveLevel(const std::string &levelName)
{
	gameInfo.setLocalString("DEBUG:EditorUtils::lastLoadedLevel", levelName);
	gameInfo.Save(false);
	if(gameInfo.getGlobalBool("iphone5_scale", false))
	{
		Assert2(false, "Используются настройки растянутого поля iphone5. Сохранение не возможно");
		return;
	}
	
	std::string  full_path = "Levels/"+levelName+".xml";
	Xml::TiXmlDocument levelDoc(full_path.c_str());
	Xml::TiXmlElement level_real("Level");
	levelDoc.InsertEndChild(level_real);
	Xml::TiXmlElement *level = levelDoc.RootElement();
	SaveLevelXml(level, levelName);
	SortAttributes(level);	//сортируем атрибуты элементов чтобы не было разночтений
	levelDoc.SaveFile();
}

void GameField::SaveLevelXml(Xml::TiXmlElement *level, std::string levelName)
{	
	//по алфавиту!
	level->SetAttribute("name", levelName.c_str());

	std::string scriptName = Tutorial::luaTutorial.GetScriptName();
	if( !scriptName.empty() ) {
		level->SetAttribute("script", scriptName.c_str());
	}

	Gadgets::levelColors.Save(level);

	std::vector<int> bonusRs;
	IPoint posLevel(GameSettings::FIELD_MAX_HEIGHT,GameSettings::FIELD_MAX_WIDTH);
	IPoint endLevel(0, 0);
	for(int i = 1; i < GameSettings::FIELD_MAX_WIDTH + 1; i++)
	{
		for(int j = 1; j < GameSettings::FIELD_MAX_HEIGHT + 1; j++)
		{
			if (!Game::isBuffer(GameSettings::gamefield[i][j]))	
			{
				if(posLevel.x > i)
				{
					posLevel.x = i;
				}
				if(posLevel.y > j)
				{
					posLevel.y = j;
				}
				if(endLevel.x < i)
				{
					endLevel.x = i;
				}
				if(endLevel.y < j)
				{
					endLevel.y = j;
				}
			}
		}
	}

	posLevel.x -= 2;
	posLevel.y -= 2;
	GameSettings::fieldWidth = endLevel.x;
	GameSettings::fieldHeight = endLevel.y;

	Xml::TiXmlElement *pLevel = level->InsertEndChild(Xml::TiXmlElement("PosLevel"))->ToElement();
	pLevel->SetAttribute("x", posLevel.x);
	pLevel->SetAttribute("y", posLevel.y);

	//for (int j = GameSettings::fieldHeight; j>=posLevel.y; j--)
	//{
	Xml::TiXmlElement *elem = level->InsertEndChild(Xml::TiXmlElement("Line"))->ToElement();
	for (int j = posLevel.y; j <= GameSettings::fieldHeight; j++) //Сохраняем снизу вверх
	{
		std::string  s;
		
		// Used chars: 
		// e, B, H, C, V, U, R, L, 
		// D, S, *, n, N, #, -, +, 
		// m, M, 0, 1, 2, 3, 4, 5, 
		// 6, 7, ., I, F, Q, ,, `,
		// ^, !

		// Chameleons:
		// $, 9, a, A, 8, k, K, ?

		for (int i = posLevel.x; i<GameSettings::fieldWidth+1; i++)
		{
			Game::Square* sq = GameSettings::gamefield[i+1][j+1];
			if (Game::isBuffer(sq))
			{
				if(sq == Game::bufSquareNoChip)
					s += ",";
				else if(sq == Game::bufSquareNoLicorice)
					s += "`";
				else if(sq == Game::bufSquareNoTimeBomb)
					s += "^";
				else if(sq == Game::bufSquareNoBomb)
					s += "!";
				else
					s += ".";
			} 
			else 
			{	
				s += sq->Serialize();
			}
		}		
		elem->InsertEndChild(Xml::TiXmlText(s.c_str()));
		if(j != GameSettings::fieldHeight)
		{
			elem = level->InsertBeforeChild(elem, Xml::TiXmlElement("Line"))->ToElement();		
		}
	}

	// запишем инфу о клеточках, которые cчитаютcя проницаемыми для энергии c точки зрения cтрелки, указывающей путь к приёмнику
	Xml::TiXmlElement *recSqRoot = level->InsertEndChild(Xml::TiXmlElement("RecArrowSquares"))->ToElement();
	bool find = false;
	for (int i = 0; i<GameSettings::fieldWidth; i++)
	{
		for (int j = 0; j<GameSettings::fieldHeight; j++)
		{
			if (GameSettings::recfield[i+1][j+1] && Game::isBuffer(GameSettings::gamefield[i+1][j+1]))
			{
				find = true;
				Xml::TiXmlElement *elem = recSqRoot->InsertEndChild(Xml::TiXmlElement("Square"))->ToElement();
				elem->SetAttribute("x", i);
				elem->SetAttribute("y", j);
			}
		}
	}
	if(!find)
	{
		level->RemoveChild(recSqRoot);
	}

	Gadgets::Save(level);
}

void GameField::BlockField(bool resetChipSequence)
{
	if (_chipSeq.size() > 0 && resetChipSequence)
	{
		ResizeChipSeq(0, false);
		MouseUp(Core::mainInput.GetMousePos());
	}

	_blocked++;
}

void GameField::UnblockField()
{
	_blocked--;
}

bool GameField::isBlocked() const
{
	return _blocked > 0 || Game::Square::HAS_FLYING_SQUARES;
}

void GameField::ComputeFieldRect()
{
	GameSettings::fieldWidth = 0;
	GameSettings::fieldHeight = 0;
	size_t count = GameSettings::squares.size();
	for (size_t i = 0; i < count; i++)
	{
		Game::Square* square = GameSettings::squares[i];
		GameSettings::fieldWidth = std::max(GameSettings::fieldWidth, square->address.GetCol());
		GameSettings::fieldHeight = std::max(GameSettings::fieldHeight, square->address.GetRow());
	}
	GameSettings::fieldWidth += 2;
	GameSettings::fieldHeight += 2;
}

Message GameField::QueryState(const Message& message) const
{
	if (message.getPublisher() == "GetSelectRyushkaName") {
		Gadgets::ryushki.QueryState(message);
	}
	return Message();
}

void GameField::AcceptMessage(const Message &message)
{
	Editor_AcceptMessage(message);

	if(message.is("OnSelectECombobox", "Styles")){
		FieldStyles::SetStyle(Gadgets::levelSettings.getString("Style"));
		FieldStyles::current->Upload();
		ApplyFieldStyle();
	} else if (message.is("LoadLevelSettings")) {
		LoadLevelSettings();
	} else if(message.is("PrepareLevel")) {
		_level = Gadgets::LoadLevelDescription();
	} else if (message.is("DebugLua")) {
		std::string mData = message.getData();
		return;
	} else if (message.is("InitPostEffects")) {
		initPostEffects();
		return;
	} else if (message.is("SaveClickMessage"))
	{
		std::string  to =  message.getVariables().getString("to");
		std::string  str =  message.getVariables().getString("str");

		if (to == "TutorialStone")
		{
			Gadgets::ryushki._selectedRyushka->setStringData(str);
		}
	}
	else if(message.is("RemoveEditor"))
	{
		Core::resourceManager.GetOrCreateGroup("Editor")->EndUse();
		Core::messageManager.putMessage(Message("CloseEditor"));
		if(true)
		{
			//_introController = new StartFieldMover();
			//Game::AddController(_introController);
			if(_introController)
			{
				_introController->Start();
			}else{
				Assert(false);
			}
			_timeFromLastMove = 0.2f;
		}else{
			OpenEnergySource();
		}
		return;
	}
	else if (message.is("Pause"))
	{
		_paused = true;
		PauseAllControllers(true);
		//Если сейчас используем буст, то цепочку очищать нельзя.
		//(поломается отрисовка зоны поражения буста)
		if (_currentActiveBoost.expired()) {
			ClearChipSeq(false);
		}
	} 
	else if (message.is("Continue"))
	{
		_paused = false;
		PauseAllControllers(false);
	}
	else if (message.is("BlockField"))
	{
		Assert(false);
		BlockField();
	} 
	else if (message.is("UnblockField"))
	{
		Assert(false);
		UnblockField();
	}
	else if (_endLevel.AcceptMessage(message))
	{
		return;
	}
	else if (message.is("MouseDown"))
	{
		MouseDown(Core::mainInput.GetMousePos());
	} 
	else if (message.is("LoadLevel"))
	{
		LoadLevelRapid(message.getData());
		return;
	}
	else if (message.is("RestartLevel"))
	{
		_restoredGame = false;
		gameInfo.getGlobalBool("GameSaved", false);
			
		AcceptMessage(Message("StartLevel"));
	}
	else if (message.is("OpenEnergySource"))
	{
		if(_introController){
			_introController->Start();
		} else {
			MyAssert(false);
		}
		//OpenEnergySource();
	}else if(message.is("AddDelayMessage")){
		std::string  msg = message.getVariables().getString("msg");
		std::string  data = message.getVariables().getString("data");
		float delay = message.getVariables().getFloat("delay");
		Game::AddController(new DelaySendMessage("GameLayer", "GameField", Message(msg, data), delay, this));
	}else if (message.is("ActivateRyushka")){
		Gadgets::ryushki.AcceptMessage(message);
	}
	else if (message.is("StartLevel"))
	{
		std::string level_name = message.getData();
		if (level_name.empty()) {
			level_name = levelsInfo.GetLevelName();
		}

		LoadLevelAndRun(level_name);
		if(!_startSoundDisable){
			Game::AddController(new DelaySendMessage("GameLayer", "GameField", "PlayStartSound", 4.2f, this));
		}
		return;
	}
	else if (message.is("ClearLevel"))
	{
		GameSettings::fieldWidth = GameSettings::FIELD_MAX_WIDTH;
		GameSettings::fieldHeight = GameSettings::FIELD_MAX_HEIGHT;
		
		for(size_t i = 0; i < 4; i++)
		{
			_squares_layer[i].clear();
		}
		GameSettings::ClearSquares();

		Gadgets::Clear();
		_maybeMoveGadget.Clear();
		
		for(size_t i = 0; i < Gadgets::editor_makers.size();i++)
		{
			Gadgets::editor_makers[i]->Clear();
		}

		levelBorder.Clear();
		_introController = NULL;
		KillAllControllers();
		KillAllEffects();

		_chipSeqAffected.clear();
		_chipSeqAffectedVisual.clear();
		_bonusCascade.clear();

		return;
	}
	else if(message.is("ChangeCounter"))
	{
		if( Gadgets::levelSettings.getString("LevelType") == "moves" ) {
			Match3GUI::ActCounter::ChangeCounter(message.getIntegerParam());
		} else {
			Match3GUI::TimeCounter::AddTime((float)message.getIntegerParam());
		}
	}
	else if(message.is("CancelCurrentBoost"))
	{
		boostList.CancelCurrentBoost(_currentActiveBoost);
		_currentActiveBoost = BoostList::WeakPtr();
	}
	else if(message.is("RunCurrentBoost"))
	{
		boostList.RunCurrentBoost(_currentActiveBoost);
		_currentActiveBoost = BoostList::WeakPtr();
	}
	else if(message.is("RunBoost"))
	{
		std::string boostname = message.getVariables().getString("name");
		for(std::list<BoostList::WeakPtr>::iterator itr = _boostBeforeStart.begin(); itr != _boostBeforeStart.end(); ++itr)
		{
			BoostList::HardPtr currentBoost = (*itr).lock();
			if (currentBoost->getName() == boostname && currentBoost->getBuyBeforeLevel())
			{
				if (currentBoost->IsInterfaceBoost()) //если интерфейсный буст (!!! должен быть только один в начале уровня, иначе поломается)
				{
					GameField *gamefield = GameField::Get();

					if (gamefield->_currentActiveBoost.expired())
					{
						//надо запустить интерфейсный буст по полной программе
						//имитируем вызов по кнопке буста чтобы в луа тоже все отработало
						//lua покажет панельки и в свою очередь вызовет UseBoostInstantly чтобы буст инициализировался
						//в общем все сложно как то, но по другому видимо никак
						Message message ("StartUseBoost");
						message.getVariables().setString("boostName", currentBoost->getName());
						Core::LuaCallVoidFunction("GameFunc", message);
					} else {
						Assert2(false, "more than one interface boost at start of level");
					}
				} else {  //неинтерфейсный просто запускаем и уменьшаем запас
					currentBoost->useBonusAndDecCount();
				}
			}
		}
	}
	else if(message.is("BoostsBeforeStartFinished"))
	{
		if(!_boostBeforeStart.empty()) {
			UnblockField();
			_boostBeforeStart.clear();
		}

		Core::mainScreen.RemoveLayer("BoostersTransition");

		// Туториал на бусты в поле
		bool isTutorial = gameInfo.needBoostTutorial(_level+1, 2);
		if (isTutorial) {
			std::string name = gameInfo.getBoostTutorialName();

			// даём 4 бесплатных буста
			gameInfo.setLocalInt("Have" + name, 4);
			Core::LuaDoString("match3Panel:boostRefresh(false)");

			// считаем буст показанным
			gameInfo.setBoostTutorialShown(name);

			// вызываем lua функцию с туториалом (tutorial.lua)
			Core::LuaCallVoidFunction(("showTutorial" + name).c_str());
		}
	}
	else if(message.is("GetChipsWithHang")) 
	{
		std::vector<IPoint> vec;
		//находим все фишки с бонусом
		for(int x = 0; x < Game::activeRect.Width();x++)
		{
			for(int y = 0; y < Game::activeRect.Height();y++)
			{
				Game::FieldAddress fa = Game::FieldAddress(Game::activeRect.x + x, Game::activeRect.y + y);
				Game::Square *sq = GameSettings::gamefield[fa];
				if (sq && Game::isVisible(sq) && sq->GetChip().HasHang()) 
				{
					vec.push_back(fa.ToPoint());
				}
			}
		}
		if (!vec.empty()) {
			Message msg = Message("TutorialCells");
			int count = (int)vec.size();
			msg.getVariables().setInt("count", count);
			for (int i = 0; i < count; i++) {
				msg.getVariables().setInt("x"+utils::lexical_cast(i+1), vec[i].x);
				msg.getVariables().setInt("y"+utils::lexical_cast(i+1), vec[i].y);
			}
			Core::LuaCallVoidFunction("fillTutorialCells", msg);
		}
	}
	else if(message.is("toActiveBoost"))
	{
		std::string msg = message.getData();
		if (!GameField::Get()->_currentActiveBoost.expired()) {
			BoostList::HardPtr currentActiveBoost = GameField::Get()->_currentActiveBoost.lock();
			currentActiveBoost->AcceptMessage(Message(msg));
		}
	}
}

std::string GameField::GetCurrentBGMTrack(int type)
{
	return "Match3Earth";
}

void GameField::KillAllEffects()
{
	_effCont.KillAllEffects();
	_effTopCont.KillAllEffects();
	_effUnderChipsCont.KillAllEffects();
	_effContUp.KillAllEffects();
	_effContUpField.KillAllEffects();
	//_energyEffectContainer.KillAllEffects();
}

void GameField::PauseAllControllers(bool pause)
{
	GameFieldControllerList::iterator i = _controllers.begin();
	GameFieldControllerList::iterator e = _controllers.end();

	for (; i != e; ++i)
	{
		(*i) -> paused = pause;
	}
}

void GameField::KillAllControllers()
{
	Game::KillControllers("");
	_seqFires = NULL;
	_cellScores.clear();
}

int GameField::GetActualChipSeqColor() const
{
	Assert(_chipSeq.size() > 0);
	return _chipSeqColor;
}

void GameField::ReleaseTargets()
{
	_screenCopy1Holder.Purge();
	_screenCopy2Holder.Purge();
	_energyEffectTarget.Purge();
	_lcEffectTarget.Purge();
	_waves.clear();
}

void GameField::ReleaseGameResources()
{
	ReleaseTargets();
	Energy::field.Release();

	GameSettings::ClearSquares();
}

void GameField::initPostEffects()
{
	// Узнаём, поддерживаютcя ли шейдеры 2 верcии на данной видеокарте
	if( !Render::device.IsVertexShaderSupported(2, 0) || !Render::device.IsPixelShaderSupported(2, 0))
	{
		Log::Fatal("Shaders not supported on this device");
	}
	
	_wavesShader = Core::resourceManager.Get<Render::ShaderProgram>("PostEffectWaves");

	_noise3d = Core::resourceManager.Get<Render::Texture>("NoiseVolume");
	_noise3d->setAddressType(Render::Texture::REPEAT);

	_energyDust = Core::resourceManager.Get<Render::Texture>("EnergyDust");
	_energyDust->setAddressType(Render::Texture::REPEAT);

	_energyHazeShader = Core::resourceManager.Get<Render::ShaderProgram>("EnergyHaze");

	_LCEffectShader = Core::resourceManager.Get<Render::ShaderProgram>("LCEffect");

	_energyShaderMaterial->SetShaderProgram(_energyHazeShader);
	_energyShaderMaterial->SetTexture(_energyDust, 0, 0);
	_energyShaderMaterial->SetTexture(&_energyEffectTarget, 1, 0);
	_energyShaderMaterial->SetTexture(_noise3d, 2, 0);

	ShaderMaterial::ShaderParameters& vsParam = _energyShaderMaterial->GetVSParameters();

	float texSize[4] = {_energyDust->getBitmapRect().width / 256.0f, _energyDust->getBitmapRect().height / 256.0f,
		_noise3d->getBitmapRect().width / 256.0f, _noise3d->getBitmapRect().height / 256.0f};
	vsParam.setParam("tex_size", texSize, 4);

	//SetEnergyShaderConstants();
#ifndef ENGINE_TARGET_WIN32
	float xscale = (float)GameSettings::FIELD_SCREEN_CONST.width / (float)Int::Pow2(GameSettings::FIELD_SCREEN_CONST.width);
	float yscale = (float)GameSettings::FIELD_SCREEN_CONST.height / (float)Int::Pow2(GameSettings::FIELD_SCREEN_CONST.height);
#else
	float xscale = 1.0f, yscale = 1.0f;
#endif
    vsParam.setParam("screen_texture_scale", math::Vector4(xscale,yscale,1,1));
}

void GameField::renderWaves()
{
	if (_waves.size() > 0)
	{
		float globalTransform[4];
		float w_vis = Render::device.IsPower2Required() ? Int::Pow2(GameSettings::FIELD_SCREEN_CONST.width) : GameSettings::FIELD_SCREEN_CONST.width; 
		float h_vis = Render::device.IsPower2Required() ? Int::Pow2(GameSettings::FIELD_SCREEN_CONST.height) : GameSettings::FIELD_SCREEN_CONST.height;
		
		globalTransform[0] = w_vis + 0.f;
		globalTransform[1] = h_vis + 0.f;
		globalTransform[2] = GameSettings::fieldX;
		globalTransform[3] = GameSettings::fieldY;

		RenderTargetHolder *active = _screenCopy2;
		RenderTargetHolder *texture = _screenCopy1;

		for (size_t i = 0, size = _waves.size() ; i < size ; ++i)
		{
			active->BeginRendering(Color(0, 0, 0, 0));

			_wavesShader->Bind();
			_wavesShader->setVSParam("xyAF", (const float *)&(_waves[i].getGPUData().xyAF), 4);
            _wavesShader->setVSParam("OffsetNNN", (const float *)&(_waves[i].getGPUData().OffsetNNN), 4);
			_wavesShader->setVSParam("globalTransform", globalTransform, 4);		

			Render::device.SetBlend(false);
            FRect UV(0, 1, 0, 1 );
			texture->Bind(0, Render::STAGE_PROJECTED);            
            texture->TranslateUV(UV);  
            Render::DrawRect(texture->GetBitmapRect(), UV);
			Render::device.SetBlend(true);
			active->EndRendering();

			_wavesShader->Unbind();
			// меняем текcтуры
			std::swap(active, texture);
		}
		_screenCopy1 = texture;
		_screenCopy2 = active;
	}
}

void GameField::addWave(const IPoint &position, float amplitude, float frequency, float velocity, float T0, float T1, float T2)
{
	_waves.push_back(Game::PostEffectWave());
	Game::PostEffectWave &wave = _waves.back();
	wave.setPosition(position);
	wave.setTiming(T0, T1, T2);
	wave.setAmplitude(amplitude);
	wave.setFrequency(frequency);
	wave.setVelocity(velocity);
}

void GameField::updateWaves(float dt)
{
	for (size_t i = 0 ; i < _waves.size() ; ++i)
	{
		_waves[i].update(dt);
		if (!_waves[i].isActive())
		{
			_waves.erase(_waves.begin() + i);
			--i;
		}
	}
}

bool GameField::IsStandby() const
{
	return Game::chipsStandby && !_fieldMoving && !_isCurrentMoveActive && !Match3::IsActive();
}

void GameField::Editor_AcceptMessage(const Message &message)
{
	if(!gameInfo.IsDevMode())
	{
		return;
	}
	if(message.is("LoadLevelForEdit"))
	{
		LoadLevelForEdit(message.getData());
		return;
	}
	else if (message.is("SaveLevel"))
	{
		SaveLevel(message.getData());
		return;
	}
	else if(message.is("ReleaseGameResources"))
	{
		ReleaseGameResources();
	}
	else if(message.is("PreloadGameResources"))
	{
		if( !_resourcePreloaded)
		{
			// асинхронно начнем загружать некоторые матч3 ресурсы в тот момент,
			// когда открыли на карте стартовую панельку, чтобы быстрее потом загрузить уровень
			Core::resourceManager.GetOrCreateGroup("GameLayerPreload")->BeginUse(ResourceLoadMode::Async);
			_resourcePreloaded = true;
		}
	}
	else if(message.is("CancelPreloadResources"))
	{
		if( _resourcePreloaded) {
			Core::resourceManager.GetOrCreateGroup("GameLayerPreload")->EndUse(ResourceLoadMode::Async);
			_resourcePreloaded = false;
		}
	}
	else if (message.is("KeyPress"))
	{
		int key = utils::lexical_cast <int> (message.getData());

		if (key == 'c' || key == 'e' || key == 'C' || key == 'E')
		{
			if (!EditorUtils::editor)
			{
				EditorUtils::editor = true;

				bool on_screen = Core::mainScreen.isLayerOnScreen("EditorLayer");
				if(!on_screen) {
					Log::Info("Show Editor");
					Core::messageManager.putMessage(Message("ShowEditor"));
				} else {
					Log::Info("Editor layer already on screen");
					Assert(false);
				}

				LoadDescriptions();

				Core::resourceManager.GetOrCreateGroup("Editor")->BeginUse();
				MM::manager.StopTrack();

				std::string level_name = EditorUtils::lastLoadedLevel;

				LoadLevelForEdit(level_name);
				InitLevelAfterLoad(true);
				Gadgets::wallDrawer.InitEditor();
				Energy::field.Release();
				
				if(gameInfo.getGlobalString("Editor::last_loaded_level", "") == level_name)
				{
					IPoint pos = gameInfo.getGlobalPoint("Editor::last_field_pos");
					GameSettings::fieldX = pos.x + 0.f;
					GameSettings::fieldY = pos.y + 0.f;
					Game::UpdateVisibleRects();
				}
			}

			return;
		}
		else if(key == -0x21/*VK_PRIOR*/)
		{
			int current_level = gameInfo.getLocalInt("current_level", 0);
			current_level = math::clamp(0, 1000, current_level + 1);
			gameInfo.setLocalInt("current_level", current_level);
			std::string level_name = levelsInfo.GetLevelName(current_level);
			if(!level_name.empty())
			{
				AcceptMessage(Message("StartLevel", level_name));
				OpenEnergySource();
			}
		}
		else if(key == -0x22/*VK_NEXT*/)
		{
			int current_level = gameInfo.getLocalInt("current_level", 0);
			int prev = current_level;
			current_level = math::clamp(0, 1000, current_level - 1);
			if(prev != current_level)
			{
				gameInfo.setLocalInt("current_level", current_level);
				std::string level_name = levelsInfo.GetLevelName(current_level);
				if(!level_name.empty())
				{
					AcceptMessage(Message("StartLevel", level_name));
					OpenEnergySource();
				}
			}
		}
		else if(key == 'm')
		{
			Match3GUI::ActCounter::ChangeCounter(20);
			Match3GUI::TimeCounter::AddTime(20.0f);
		}
		else if (key == 'p')
		{
			levelsInfo.LoadLevelMap();
		}
		else if (key == 'z')
		{
			// Этот вызов перезагрузит текстуры для текущего стиля поля. 
			// Нужно для художников, чтобы смотреть без лишних действий
			// как будет выглядеть редактируемый в данный момент стиль

			ApplyFieldStyle();
			levelBorder.Calculate();
		}
		else if(key == 'g')
		{
			_maybeMoveGadget.NeedUpdate();
			_maybeMoveGadget.Find();
		}
		else if (key == 'h')
		{
			// Перезагружаем стили уровней.
			// Там имена текстур и цвета
			FieldStyles::Load(NULL);
		}
		else if(key == 'q')
		{
			effectPresets.ReloadBinaryEffects("Effects\\Match3Effect.pbi");
			effectPresets.ReloadBinaryEffects("Effects\\Hang.pbi");
			effectPresets.ReloadBinaryEffects("Effects\\AltarsEffects.pbi");
			effectPresets.ReloadBinaryEffects("Effects\\ground.pbi");
			Gadgets::receivers.ReloadEffect();
		}
		else if(key == '+')
		{
			if(!_endLevel.IsRunning())
			{				
				Match3GUI::LootPanel::AddScore(Gadgets::levelSettings.getInt("Star3"));
				_endLevel.Run(true);
			}
		}
		else if(key == ' ')
		{
			if(!_endLevel.IsRunning())
			{
				if (Core::mainInput.IsControlKeyDown()) {
					_endLevel.SetLoseLifeReason(LevelEnd::OUT_OF_MOVES);
				} else if(Core::mainInput.IsShiftKeyDown()) {
					_endLevel.Run(false);
				} else {
					_endLevel.Run(true);
				}
			}
		}
		else if(key == 't')
		{
			NoMovesReshuffle();
		}
		else if(key == 'o' && gameInfo.IsDevMode())
		{
			EditorUtils::debug_scale = !EditorUtils::debug_scale;
		}
		else if(key == 'w' && gameInfo.IsDevMode())
		{
			EditorUtils::draw_debug = !EditorUtils::draw_debug;
		}
		else if(key == 'v' && gameInfo.IsDevMode())
		{
			Utils2D::PictureCellGenerator generage;
		}

		if (EditorUtils::editor)
		{
			if (key == 'b')
			{
				_needFon = !_needFon;
				return;
			}
		}
	}
}

// Функция для мгновенного удаления cтены и заполнения клетки энергией.
// Удаляет также лёд и камень, еcли еcть. Нужна для гейм-дизайнеров, чтобы
// проходить уровень в игре, раcчищая путь левой мышкой c зажатым Control

void GameField::Editor_InstantWallRemove(const Game::FieldAddress& address)
{
	Game::Square *s = GameSettings::gamefield[address];

	if ( !Game::isBuffer(s) )
	{
		if (s->IsStone())
		{
			s->SetStone(false);
			s->SetFake(false);
		}

		if (s->IsIce())
		{
			s->ice = 0;
		}
		if(s->IsSand())
		{
			s->SetSand(false, true);
		}
			
		if (s->GetWall() > 0 || s->GetWood() > 0 || s->IsEnergyWood())
		{
			s->SetWall(0);
			if(s->GetWood() > 0 || s->IsEnergyWood())
			{
				s->SetWood(0);
				s->SetEnergyWood(false);
				Match3::RunFallColumn(address.GetCol());
			}
			Energy::field.UpdateSquare(address);
		}
	}
}

void GameField::UpdateScores(float dt)
{
	for(CellScores::iterator itr = _cellScores.begin(); itr != _cellScores.end(); )
	{
		itr->second.delay -= dt;
		if( itr->second.delay <= 0.0f ) {
			RunScore(itr->first, itr->second.score, 0.0f, itr->second.scale, itr->second.color, 1.0f, "");
			_cellScores.erase(itr++);
		} else {
			++itr;
		}
	}
}

void GameField::AddScore(Game::FieldAddress fa, int score, float delay, float scale, Color color)
{
	std::pair<CellScores::iterator, bool> result = _cellScores.insert( std::make_pair(fa, CellScoreInfo(score, delay + 0.2f, scale, color)) );
	if( !result.second ) {
		result.first->second.score += score;
		result.first->second.scale = std::max(result.first->second.scale, scale);
	}
}

void GameField::RunScore(Game::FieldAddress fa, int score, float delay, float scale, Color color, float duration, const std::string &effName)
{
	FPoint pos = GameSettings::CellCenter(fa.ToPoint());
	Game::AddController(new AddScoreEffect(pos, score, scale, color, duration, delay, effName));
}

LevelObjective::Type GameField::GetLevelObjective() const
{
	return _levelObjective;
}

bool GameField::IsLevelObjectiveCompleted() const
{
	if(_levelObjective == LevelObjective::RECEIVERS)
	{
		return Gadgets::receivers.ActiveCount() >= Gadgets::receivers.TotalCount();
	}
	else if(_levelObjective == LevelObjective::DIAMONDS)
	{
		return Game::diamondsFound >= Gadgets::levelSettings.getInt("LevelObjectiveAmount", 0);
	}
	else if(_levelObjective == LevelObjective::BEARS)
	{
		int total = Gadgets::levelSettings.getInt("LevelObjectiveAmount", 0);
		if( total == 0 )
			total = Gadgets::bears.TotalBears();
		return Gadgets::bears.FoundBears() >= total;
	}
	else if(_levelObjective == LevelObjective::SCORE)
	{
		bool movesOrTimeExpired = (Match3GUI::ActCounter::GetCounter() <= 0) || (Match3GUI::TimeCounter::GetTime() <= 0.0f);
		return movesOrTimeExpired && (Match3GUI::LootPanel::GetScore() >= Gadgets::levelSettings.getInt("LevelObjectiveAmount", 0));
	}
	else if(_levelObjective == LevelObjective::ENERGY)
	{
		return Gadgets::snapGadgets.IsEnergyLevelFinished();
	}
	else
	{
		Assert(false);
		return false;
	}
}

std::string GameField::GetLevelObjectiveText() const
{
	int active;
	int total;
	if(_levelObjective == LevelObjective::RECEIVERS)
	{
		active = Gadgets::receivers.ActiveCount();
		total = Gadgets::receivers.TotalCount();
	}
	else if(_levelObjective == LevelObjective::DIAMONDS)
	{
		active = Game::diamondsFound;
		total = Gadgets::levelSettings.getInt("LevelObjectiveAmount", 0);
	}
	else if(_levelObjective == LevelObjective::BEARS)
	{
		active = Gadgets::bears.FoundBears();
		int t = Gadgets::levelSettings.getInt("LevelObjectiveAmount", 0);
		total = (t > 0) ? t : Gadgets::bears.TotalBears();
	}
	else if(_levelObjective == LevelObjective::SCORE)
	{
		return Gadgets::levelSettings.getString("LevelObjectiveAmount");
	}
	else if(_levelObjective == LevelObjective::ENERGY)
	{
		active = SnapGadgetReleaseBorderElement::RELEASED_SQUARES;
		total = SnapGadgetReleaseBorderElement::LEVEL_SQUARES_COUNT;
	}
	return utils::lexical_cast(active) + "/" + utils::lexical_cast(total);
}
