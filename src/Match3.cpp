#include "stdafx.h"
#include "Match3.h"
#include "Game.h"
#include "Match3Gadgets.h"
#include "CombinedBonus.h"
#include "GameField.h"
#include "SnapGadgetsClass.h"
#include "CellWalls.h"

namespace Match3
{

static const float CHIP_VELOCITY = 10.0f; // squares per second
static const float CHIP_ACC = 15.0f;
static const float CHIP_PAUSE = 0.1f;

typedef std::set<int> FallColumns;

static FallColumns _fallColumnsMove;   // колонки, которые нужно обновить только после окончания хода
static FallColumns _fallColumnsUpdate; // колонки, которые нужно обновить в текущем кадре
static FallColumns _fallColumnsAdd;    // дополнительные столбцы, которые изменяются в процессе просчета падения фишек, будут обработаны на следующем кадре

static bool newChipsUnderSolid = false;  // генерировать ли новые фишки под плитами, шоколадом и прочими подобными клетками
static bool newChipsOnUpperEdge = false; // генерировать ли новые фишки на верхней границе активной области
static bool flyingCanHost = false;

static int licoricePercent = 0;
static int timeBombPercent = 0;
static int licoriceLimit = 0;
static int timeBombLimit = 0;
static int timeBombMoves = 10;
static int lightPercent = 0;

static int bombPercent = 0; //вероятность появления фишки с бомбой
static int bombLimit = 0;   //лимит фишек с бомбами


static int minRow, maxRow;
static int minCol, maxCol;
static int paused = 0;

bool CanSpawnLicorice(Game::Square *sq, Game::Square *srcSquare)
{
	if( srcSquare == Game::bufSquareNoLicorice || sq->IsIce() || sq->GetChip().HasHang())
		return false;

	return true;
}

bool CanSpawnTimeBomb(Game::Square *sq, Game::Square *srcSquare)
{
	if( srcSquare == Game::bufSquareNoTimeBomb || sq->IsIce())
		return false;

	return true;
}

bool CanSpawnBomb(Game::Square *sq, Game::Square *srcSquare)
{
	if( srcSquare == Game::bufSquareNoBomb || sq->IsIce())
		return false;

	return true;
}

// генерирует новую фишку в указанном квадрате. для корректной работы заранее вызвать Game::CountLicoriceAndBombsInTargetRect();
void GenerateChipInSquare(Game::Square *sq, float alpha, Game::FieldAddress srcCell)
{
	sq->GetChip().ClearOffset();
	sq->GetChip().SetAdapt(false);
	Gadgets::ChipSource *src = Gadgets::chipSources.GetSource(srcCell);
	if( src ) {
		src->GenerateChipInSquare(sq);
	} else {
		int rnd = math::random(0, 99);
		int p2 = licoricePercent + timeBombPercent;
		int p3 = p2 + lightPercent;
		int p4 = p3 + bombPercent ;
		Game::Square *srcSquare = GameSettings::gamefield[srcCell];
		if( rnd < licoricePercent && CanSpawnLicorice(sq, srcSquare) && Game::LicoriceOnScreen() < licoriceLimit ) {
			// лакрица
			sq->GetChip().SetLicorice();
			Game::LicoriceInc();
		} else if( rnd >= licoricePercent && rnd < p2 && CanSpawnTimeBomb(sq, srcSquare) && Game::TimeBombsOnScreen() < timeBombLimit ) {
			// тайм-бомба
			sq->GetChip().SetColor( Gadgets::levelColors.GetRandom() );
			sq->GetChip().SetTimeBomb(timeBombMoves);
			Game::TimeBombInc();
		} else if( rnd >= p2 && rnd < p3 ) {
			// светлячок
			sq->GetChip().SetColor( Gadgets::levelColors.GetRandom() );
			sq->GetChip().SetLight();
		} else if( rnd >= p3 && rnd < p4 && CanSpawnBomb(sq, srcSquare) && Game::BombsOnScreen() < bombLimit ) {
			// фишка с бомбой
			sq->GetChip().SetColor( Gadgets::levelColors.GetRandom() );
			sq->GetChip().GetHang().MakeBomb(1,1);
			Game::BombInc();
		} else {
			// обычная фишка
			sq->GetChip().SetColor( Gadgets::levelColors.GetRandom() );
		}
	}
	sq->GetChip().SetAlpha(alpha, alpha, 1.0f);
}

// генерирует новую фишку, падающую из квадрата from в квадрат to
void FallNewChipToSquare(Game::FieldAddress from, Game::Square *to, float pause)
{
	//подсчитываем сколько кого есть на экране. именно сейчас чтобы вновь появившаяся не сбивала счетчик
	Game::CountLicoriceAndBombsInTargetRect();
	
	GenerateChipInSquare(to, 0.0f, from);

	FPoint from_pos = FPoint(from.ToPoint() * GameSettings::SQUARE_SIDE);
	FPoint offset(from.ToPoint() * GameSettings::SQUARE_SIDE);
	to->GetChip().MoveBy(from_pos - to->GetCellPos());
	to->GetChip().RunFall(pause, CHIP_VELOCITY * GameSettings::SQUARE_SIDEF, CHIP_ACC * GameSettings::SQUARE_SIDEF);
}

// запускает падение фишки из квадрата from в квадрат to
void FallFromSquareToSquare(Game::Square *from, Game::Square *to, float pause, Game::Square *med = nullptr)
{
	if(from->address.GetRow() > maxRow) {
		// не трогаем и не обваливаем фишки за пределами активной области, генерируем вместо них новые
		FallNewChipToSquare(from->address, to, pause);
	} else {
		to->GetChip().ClearOffset();
		if(from->GetChip()._chipInSequence)
		{
			GameField::Get()->ResizeChipSeq(0, false);
		}
		Game::Square::SwapChips(from, to);

		FPoint pos = from->GetChip().GetPos() + from->GetCellPos() - to->GetCellPos();
		to->GetChip().SetPos(pos);
		if( med ) {
			pos = pos + med->GetCellPos() - from->GetCellPos();
			to->GetChip().AddFlyWaypoint(pos);
		}
		to->GetChip().RunFall(pause, CHIP_VELOCITY * GameSettings::SQUARE_SIDEF, CHIP_ACC * GameSettings::SQUARE_SIDEF);
	}
}

// в квадрате есть фишка, которая может упасть
bool HasChipToFall(Game::Square *sq)
{
	return sq->GetChip().IsExist() && !sq->IsIce() && !sq->GetChip().IsStarAct()
		&& sq->GetChip().GetTreasure() == 0 && sq->IsStayFly();
}

// квадрат свободен для того, чтобы в него упала фишка
bool SquareCanHostChip(Game::Square *sq)
{
	return !Game::isBuffer(sq) && !sq->GetChip().IsExist() && !sq->IsFake()
		&& !sq->IsHardStand() && (sq->IsStayFly() || flyingCanHost);
}

// могут ли падать новые клетки из указанного квадрата
bool CanGenerateChips(Game::Square *sq)
{
	return (sq == Game::bufSquare) || (sq == Game::bufSquareNoLicorice)
		|| (sq == Game::bufSquareNoTimeBomb) || (sq == Game::bufSquareNoBomb)
		|| ( newChipsUnderSolid && (sq->IsHardStand() || sq->IsIce()))
		|| (newChipsOnUpperEdge && sq->address.GetRow() >= maxRow)
		|| !sq->IsStayFly();
}

// может ли фишка пролетать через эту клетку
bool CanFallThroughSquare(Game::Square *sq)
{
	return SquareCanHostChip(sq) || sq->IsShortSquare();
}

// мешают ли стенки упасть фишке вниз
bool IsWalledDown(Game::Square *sq)
{
	return (sq->cellWallsChip & Gadgets::CellWalls::DOWN) != 0;
}

// мешают ли стенки упасть фишке по диагонали
bool IsWalledDiagonal(Game::Square *to, Game::Square *from)
{
	if(to->address.GetCol() > from->address.GetCol())
	{
		return ( (from->cellWallsChip & Gadgets::CellWalls::RIGHT) || (to->cellWallsChip & Gadgets::CellWalls::UP) )
			&& ( (from->cellWallsChip & Gadgets::CellWalls::DOWN) || (to->cellWallsChip & Gadgets::CellWalls::LEFT) );
	}
	else
	{
		return ( (from->cellWallsChip & Gadgets::CellWalls::LEFT) || (to->cellWallsChip & Gadgets::CellWalls::UP) )
			&& ( (from->cellWallsChip & Gadgets::CellWalls::DOWN) || (to->cellWallsChip & Gadgets::CellWalls::RIGHT) );
	}
	return false;
}

// запускает падение подходящей фишки из текущего столбца, если такая есть, в клетку to
void FallToSquare(Game::Square *to, float &startPause, Game::Square *dest = nullptr)
{
	Game::Square *med = nullptr;
	if( !dest )
		dest = to;
	else
		med = to;

	Game::FieldAddress fa = to->address.Up();
	while( fa.GetRow() <= GameSettings::FIELD_MAX_HEIGHT /*maxRow*/)
	{
		Game::Square *sq = GameSettings::gamefield[fa];
		if( IsWalledDown(sq) ) {
			return;
		} if( HasChipToFall(sq) ) {
			if( sq->GetChip().IsStand() )
				FallFromSquareToSquare(sq, dest, startPause, med);
			return;
		} else if( CanGenerateChips(sq) ) {
			FallNewChipToSquare(fa, dest, startPause);
			startPause += CHIP_PAUSE;
			return;
		} else if( !CanFallThroughSquare(sq) ) {
			return;
		}
		fa = fa.Up();
	}
}

// запускает диагональное падение фишки (если есть) из одного из соседних столбцов в клетку to
Game::FieldAddress FallToSquareDiag(Game::Square *to, float pause)
{
	Game::FieldAddress fa = to->address.Up();

	Game::Square *sq = GameSettings::gamefield[fa];
	while( CanFallThroughSquare(sq) && !HasChipToFall(sq) && !IsWalledDown(sq)){
		fa = fa.Up();
		sq = GameSettings::gamefield[fa];
	}
	
	if( HasChipToFall(sq) && !IsWalledDown(sq) )
		return fa.Up();

	Game::FieldAddress result = fa;

	Game::Square *short_sq = nullptr; // пролетаемая клетка, через которую потенциально можно будет запустить фишку

	while( fa.GetRow() > to->address.GetRow() )
	{
		Game::Square *sq1 = GameSettings::gamefield[fa.Left()];
		Game::Square *sq2 = GameSettings::gamefield[fa.Right()];

		if(math::random(0,1) > 0)
			std::swap(sq1, sq2);

		Game::Square *to = GameSettings::gamefield[fa.Down()];

		if( !IsWalledDiagonal(to, sq1)) {
			if( HasChipToFall(sq1) ) {
				// если фишка, которую мы хотим уронить, еще летит к своей клетке,
				// то засчитываем ее, но фактически она начнет падать после того как долетит
				if(sq1->GetChip().IsStand()) {                     
					FallFromSquareToSquare(sq1, to, pause);
					_fallColumnsAdd.insert(sq1->address.GetCol());
				}
				return fa;
			} else if( !short_sq && sq1->IsShortSquare() ) {
				short_sq = sq1;
			}
		}
		if( !IsWalledDiagonal(to, sq2)) {
			if( HasChipToFall(sq2) ) {
				if(sq2->GetChip().IsStand()) {
					FallFromSquareToSquare(sq2, to, pause);
					_fallColumnsAdd.insert(sq2->address.GetCol());
				}
				return fa;
			} else if( !short_sq && sq2->IsShortSquare() ) {
				short_sq = sq2;
			}
		}

		fa = fa.Down();
	}

	if( short_sq )
	{
		FallToSquare(short_sq, pause, to);
	}

	return result;
}

void ProcessColumn(int col)
{
	Game::FieldAddress fa(col, minRow);
	float startPause = 0.0f;

	while(fa.GetRow() <= maxRow)
	{
		Game::Square *sq = GameSettings::gamefield[fa];
		if( SquareCanHostChip(sq) ){
			FallToSquare(sq, startPause);
		}
		fa = fa.Up();
	}
}

void ProcessColumnDiag(int col)
{
	Game::FieldAddress fa(col, minRow);

	while(fa.GetRow() <= maxRow)
	{
		Game::Square *sq = GameSettings::gamefield[fa];
		if( SquareCanHostChip(sq) ){
			fa = FallToSquareDiag(sq, 0.0f);
		} else {
			fa = fa.Up();
		}
	}
}

void RunFallColumn(int col)
{
	_fallColumnsUpdate.insert(col);
}

void FallColumnsOnUpdate()
{
	if( paused <= 0 && !GameField::Get()->SelectingSequence())
	{
		for(int col : _fallColumnsUpdate)
		{
			ProcessColumn(col);
		}

		for(int col : _fallColumnsUpdate)
		{
			ProcessColumnDiag(col);
		}

		_fallColumnsUpdate = _fallColumnsAdd;
		_fallColumnsAdd.clear();
	}
}

void Init()
{
	_fallColumnsUpdate.clear();
	_fallColumnsMove.clear();
	_fallColumnsAdd.clear();

	newChipsUnderSolid = Gadgets::levelSettings.getBool("NewChipUnderHard");
	newChipsOnUpperEdge = false;

	timeBombMoves = Gadgets::levelSettings.getInt("TimeBombMoves");
	lightPercent = Gadgets::levelSettings.getInt("LightPercent");

	licoricePercent = Gadgets::levelSettings.getInt("LicoricePercent");
	timeBombPercent = Gadgets::levelSettings.getInt("TimeBombPercent");
	licoriceLimit = Gadgets::levelSettings.getInt("LicoriceLimit");
	timeBombLimit = Gadgets::levelSettings.getInt("TimeBombLimit");
	bombPercent = Gadgets::levelSettings.getInt("BombPercent");
	bombLimit = Gadgets::levelSettings.getInt("BombLimit");

	paused = 0;
}

void SetRect(int left, int right, int bottom, int top, bool genChipsOnTop)
{
	minRow = bottom;
	maxRow = top;
	minCol = left;
	maxCol = right;

	newChipsOnUpperEdge = genChipsOnTop;
}

bool IsActive()
{
	return !(_fallColumnsUpdate.empty() && _fallColumnsMove.empty());
}

void Pause()
{
	++paused;
}

void Unpause()
{
	--paused;
}

/***********************************************************************************
Реализация мгновенного заполнения поля фишками, так как если бы они упали с учетом
диагонального падения.
***********************************************************************************/

int AddressToIndex(Game::FieldAddress fa)
{
	int col = fa.GetCol() - minCol;
	int row = fa.GetRow() - minRow;
	return row * (maxCol - minCol + 1) + col;
}

Game::FieldAddress SrcFromMap(Game::FieldAddress fa, Game::FieldAddress* map)
{
	return map[ AddressToIndex(fa) ];
}

Game::FieldAddress CanGenerateChipOnStart(Game::FieldAddress fa, Game::FieldAddress prev, Game::FieldAddress *srcMap)
{
	Game::Square *sq = GameSettings::gamefield[fa];
	Game::FieldAddress src;

	if( !IsWalledDown(sq) )
	{
		if( CanGenerateChips(sq) )
			return fa;

		src = SrcFromMap(fa, srcMap);
		if( src.IsValid() )
			return src;

		if( CanFallThroughSquare(sq) && prev.IsValid() )
			return prev;
	}

	if( !IsWalledDiagonal(GameSettings::gamefield[fa.Down()], GameSettings::gamefield[fa.Left()]) )
	{
		src = SrcFromMap(fa.Left(), srcMap);
		if( src.IsValid() )
			return src;
	}

	if( !IsWalledDiagonal(GameSettings::gamefield[fa.Down()], GameSettings::gamefield[fa.Right()]) )
	{
		src = SrcFromMap(fa.Right(), srcMap);
		if( src.IsValid() )
			return src;
	}

	return Game::FieldAddress(-1,-1);
}

bool FillColumn(int col, Game::FieldAddress* map)
{
	Game::FieldAddress fa(col, maxRow);
	Game::FieldAddress src(-1,-1);
	bool chipsGenerated = false;
	while( fa.GetRow() >= minRow )
	{
		Game::Square *sq = GameSettings::gamefield[fa];

		if( sq->IsIce() )
			src = Game::FieldAddress(-2,-2); // невалидный адрес, означающий что нам все таки нужно будет сгенерировать здесь фишку
		else
			src = CanGenerateChipOnStart(fa.Up(), src, map);

		Game::FieldAddress srcFromMap = SrcFromMap(fa, map);
		if(SquareCanHostChip(sq) && (src.IsValid() || src == Game::FieldAddress(-2,-2)) && srcFromMap == Game::FieldAddress(-1,-1)){
			map[ AddressToIndex(fa) ] = src;
			chipsGenerated = true;
		}

		fa = fa.Down();
	}

	return chipsGenerated;
}

void EnsureMoveExists()
{
	// получаем ту область, которая предположительно будет видимой в начале уровня (после стартового пролета камеры по уровню)
	SnapGadget* snap = Gadgets::snapGadgets.FindFirstGadget();
	if( !snap )
		return;

	FPoint tmp(GameSettings::fieldX, GameSettings::fieldY);
	GameSettings::fieldX = snap->_snapPoint.x - GameSettings::FIELD_SCREEN_CENTER.x;
	GameSettings::fieldY = snap->_snapPoint.y - GameSettings::FIELD_SCREEN_CENTER.y;
	Game::UpdateVisibleRects();


	// проверяем есть ли ходы
	for (int x = Game::activeRect.LeftBottom().x; x <= Game::activeRect.RightTop().x; x++)
	{
		for (int y = Game::activeRect.LeftBottom().y; y <= Game::activeRect.RightTop().y; y++)
		{
			Game::FieldAddress fa(x, y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if( sq->GetChip().IsSimpleChip() && Game::CheckMaxSeq(fa, 3) ) {
				GameSettings::fieldX = tmp.x;
				GameSettings::fieldY = tmp.y;
				Game::UpdateVisibleRects();
				return;
			}
		}
	}

	// ходов нет, ищем фишки, имеющие двух соседей, перекрашиваем фишку и соседей в один цвет
	std::vector< std::vector<Game::FieldAddress> > moves;
	std::vector<Game::FieldAddress> neighbours;
	for (int x = Game::activeRect.LeftBottom().x; x <= Game::activeRect.RightTop().x; x++)
	{
		for (int y = Game::activeRect.LeftBottom().y; y <= Game::activeRect.RightTop().y; y++)
		{
			Game::FieldAddress fa(x, y);
			Game::Square *sq = GameSettings::gamefield[fa];
			if(sq->GetChip().IsSimpleChip())
			{
				neighbours.clear();
				for(size_t k = 0; k < Gadgets::checkDirsChains.count; ++k)
				{
					Game::FieldAddress nfa = fa.Shift(Gadgets::checkDirsChains.dx[k], Gadgets::checkDirsChains.dy[k]);
					if( GameSettings::gamefield[nfa]->GetChip().IsSimpleChip() )
						neighbours.push_back(nfa);
				}

				if( neighbours.size() >= 2 )
				{
					std::random_shuffle(neighbours.begin(), neighbours.end());
					neighbours.resize(2);
					neighbours.push_back(fa);
					moves.push_back( neighbours );
				}
			}
		}
	}

	if( !moves.empty() )
	{
		size_t idx = math::random(0u, moves.size()-1);
		
		Game::Square *sq0 = GameSettings::gamefield[moves[idx][0]];
		Game::Square *sq1 = GameSettings::gamefield[moves[idx][1]];
		Game::Square *sq2 = GameSettings::gamefield[moves[idx][2]];

		sq0->GetChip().SetColor( sq2->GetChip().GetColor() );
		sq1->GetChip().SetColor( sq2->GetChip().GetColor() );
	}

	GameSettings::fieldX = tmp.x;
	GameSettings::fieldY = tmp.y;

	Game::UpdateVisibleRects();
}

void FillLevel()
{
	flyingCanHost = true;

	int width = maxCol - minCol + 1;
	int height = maxRow - minRow + 1;
	// для каждой клетки поля в этом массиве будем сохранять адрес источника, на основе которого нужно будет сгенерировать (или нет) фишку в этой клетке
	Game::FieldAddress* srcMap = new Game::FieldAddress[width*height]; 
	for(int i = 0; i < width*height; ++i)
		srcMap[i] = Game::FieldAddress(-1,-1);

	bool chipsGenerating = true;
	while( chipsGenerating )
	{
		chipsGenerating = false;
		for(int i = minCol; i <= maxCol; ++i) {
			chipsGenerating |= FillColumn(i, srcMap);
		}
	}

	//предварительно считаем сколько чего уже выставлено
	Game::CountLicoriceAndBombsInTargetRect();
	
	for(int col = minCol; col <= maxCol; ++col)
	{
		for(int row = minRow; row <= maxRow; ++row)
		{
			Game::FieldAddress fa(col, row);
			Game::Square *sq = GameSettings::gamefield[fa];
			Game::FieldAddress src = srcMap[ AddressToIndex(fa) ];
			if( src.IsValid() || src == Game::FieldAddress(-2,-2))
			{
				GenerateChipInSquare(sq, 1.0f, src);
			}
		}
	}

	delete[] srcMap;

	EnsureMoveExists();

	flyingCanHost = false;
}

bool NeedPreserveChip(Game::Square *sq)
{
	const Game::ChipColor &ch = sq->GetChip();
	return ch.IsExist() &&
			(ch.IsMusor() || ch.IsChameleon() || ch.IsKey() || ch.IsEnergyBonus()
			|| ch.IsAdapt() || ch.IsLock() || sq->IsIce() || ch.IsPreinstalled());
}

void RefillRect()
{
	// очищаем видимую область и обваливаем фишки в ней, если не мешают предустановленные элементы
	for(int x = minCol; x <= maxCol; x++)
	{
		for(int y = minRow; y <= maxRow; y++)
		{
			Game::Square *sq = GameSettings::gamefield[x+1][y+1];
			if( !NeedPreserveChip(sq) )
				sq->GetChip().Reset(true);
		}
		RunFallColumn(x);
	}
}

/*****************************************************************************
 Расчет того, куда могут упасть фишки. Работает примерно как и заполнение
 поля в его первоначальном, более простом, варианте
*****************************************************************************/

bool HasChipToFallCalc(Game::Square *sq)
{
	return sq->willFallChip && !sq->IsIce() && !sq->GetChip().IsStarAct() && sq->GetChip().GetTreasure() == 0;
}

bool ChipCanAppearUnderSquare(Game::FieldAddress fa, bool prev)
{
	Game::Square *sq = GameSettings::gamefield[fa];
	Game::Square *sq_down = GameSettings::gamefield[fa.Down()];
	Game::Square *sq_left = GameSettings::gamefield[fa.Left()];
	Game::Square *sq_right = GameSettings::gamefield[fa.Right()];

	if( !IsWalledDown(sq) )
	{
		if( HasChipToFallCalc(sq) || CanGenerateChips(sq) || (CanFallThroughSquare(sq) && prev) )
			return true;
	}

	if( !IsWalledDiagonal(sq_down, sq_left) )
	{
		if( HasChipToFallCalc(sq_left) )
			return true;
	}

	if( !IsWalledDiagonal(sq_down, sq_right) )
	{
		if( HasChipToFallCalc(sq_right) )
			return true;
	}
	
	return false;
}

bool CalcColumn(int col)
{
	Game::FieldAddress fa(col, maxRow);
	bool genChips = false;
	bool chipsGenerated = false;
	while( fa.GetRow() >= minRow )
	{
		Game::Square *sq = GameSettings::gamefield[fa];

		genChips = ChipCanAppearUnderSquare(fa.Up(), genChips);
		if(!sq->willFallChip && SquareCanHostChip(sq) && (genChips || sq->IsIce())){
			sq->willFallChip = true;
			chipsGenerated = true;
		}

		fa = fa.Down();
	}

	return chipsGenerated;
}

void PrecalcFall()
{
	for(size_t i = 0; i < GameSettings::squares.size(); ++i) {
		GameSettings::squares[i]->willFallChip = GameSettings::squares[i]->GetChip().IsExist();
	}

	bool chipsFalling = true;
	while( chipsFalling )
	{
		chipsFalling = false;
		for(int i = minCol; i <= maxCol; ++i) {
			chipsFalling |= CalcColumn(i);
		}
	}
}

} // Match3 namespace