#include "stdafx.h"
#include "MaybeMoveHint.h"
#include "Match3Gadgets.h"
#include "GameFieldControllers.h"


bool CompareBySize(AddressVector *v0, AddressVector* v1)
{
	return v0->size() > v1->size();
}

//namespace Gadgets
//{
//	void DrawOneArrow(const IPoint &p0, const IPoint &p1, const float &alphaFactor, const float &factor, const float &r)
//	{
//		IRect imageRect = arrowsTex->getBitmapRect();
//		const float SIDE_ON_TEXTURE = 44.f;
//		const IRect arrowRect(-GameSettings::SQUARE_SIDE/2, -GameSettings::SQUARE_SIDE/2, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);
//
//		int x = (p0.x + p1.x + 1) * GameSettings::SQUARE_SIDE / 2;
//		int y = (p0.y + p1.y + 1) * GameSettings::SQUARE_SIDE / 2;
//
//		int dx = p1.x - p0.x;
//		int dy = p1.y - p0.y;
//		int n;
//
//		bool diagonal = (math::abs(dx) + math::abs(dy) > 1);
//
//		if (p1.x > p0.x) {
//			n = ( p1.y < p0.y ) ? 3 : 0;
//		} else if (p1.y > p0.y) {
//			n = 1;
//		} else if (p1.x < p0.x) {
//			n = 2;
//		} else {
//			n = 3;
//		}
//
//		float u1 = float (SIDE_ON_TEXTURE * n) / imageRect.width;
//		float u2 = float (SIDE_ON_TEXTURE * (n + 1)) / imageRect.width;
//		float v1 = 1.0f - float (SIDE_ON_TEXTURE) / imageRect.height;
//		float v2 = 1.0f;
//
//		float v3 = 1.0f - float (SIDE_ON_TEXTURE * 2) / imageRect.height;
//		float v4 = v3 + float (SIDE_ON_TEXTURE) / imageRect.height;	
//
//		Render::device.PushMatrix();
//		Render::device.MatrixTranslate(math::Vector3(
//			x + r / 2 * dx * (1.0f - factor),
//			y + r / 2 * dy * (1.0f - factor), 
//			0.0f));
//
//		if( diagonal )
//			Render::device.MatrixRotate( math::Vector3::UnitZ, 45.0f );
//
//		Render::BeginAlphaMul(alphaFactor);
//		arrowsTex->Draw(arrowRect, u1, u2, v1, v2);
//		Render::EndAlphaMul();
//
//		// Риcуем точку определённого цвета
//		Color chipColor = Color(255, 255, 255);
//		chipColor.alpha = math::lerp(0, 255, alphaFactor);
//
//		Render::BeginColor(chipColor);
//		arrowsTex->Draw(arrowRect, u1, u2, v3, v4);
//		Render::EndColor();
//
//		Render::device.PopMatrix();
//	}
//
//	void DrawArrowSequence(float alphaFactor, float vTime, const AddressVector &seq)
//	{
//		if (seq.size() < 2) return;
//		float r = 3.0f + 0.8f * (float) seq.size();
//		if (seq.size() < 3) r = 0.0f;
//		size_t count = seq.size();
//		for (size_t k = 0; k < count - 1; k++)
//		{
//			float factor = abs(cosf(GameSettings::fieldTimer * vTime + float (k) * 0.25f));
//			DrawOneArrow(seq[k + 0].ToPoint(), seq[k + 1].ToPoint(), alphaFactor, factor, r);
//		}
//	}
//
//	void DrawArrowSequence(float alphaFactor, float vTime, const std::vector<IPoint> &seq)
//	{
//		if (seq.size() < 2) return;
//		float r = 3.0f + 0.8f * 20.f; //(float) seq.size();
//		if (seq.size() < 3) r = 0.0f;
//		size_t count = seq.size();
//		for (size_t k = 0; k < count - 1; k++)
//		{
//			float factor = abs(cosf(GameSettings::fieldTimer * vTime + float (k) * 0.25f));
//			DrawOneArrow(seq[k + 0], seq[k + 1], alphaFactor, factor, r);
//		}
//	}
//}

MaybeMoveHint::MaybeMoveHint()
	:_needUpdate(false)
	, _allowDiagonal(true)
	, _chainsExist(true)
{
	Clear();
}

void MaybeMoveHint::Draw()
{
}

void MaybeMoveHint::DrawDebug()
{

	Render::device.SetTexturing(false);
	Render::BeginColor(Color(0, 0, 0, 50));
	Render::DrawRect(IRect(GameSettings::fieldX, GameSettings::fieldY, 1000, 1000));
	Render::EndColor();
	Render::device.SetBlendMode(Render::ADD);
	IPoint start_offset = Game::activeRect.LeftBottom();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			MaybeCell &cell = _maybeMoveCell[x][y];
			if(!cell.exist)
			{
				continue;
			}
			IPoint ipos = start_offset + IPoint(x,y);
			ipos = ipos*GameSettings::SQUARE_SIDE;

			FPoint pos = GameSettings::CELL_HALF + ipos;
			for(size_t k = 0; k < Gadgets::checkDirsChains.count;k++)
			{
				if(!cell.dirs[k])
				{
					continue;
				}
				Render::BeginColor(Color(255, 255, 0));
				Render::DrawLine(pos, pos + FPoint(Gadgets::checkDirsChains.dx[k],  Gadgets::checkDirsChains.dy[k])*GameSettings::SQUARE_SIDEF*0.5f);
				Render::EndColor();
			}
		}
	}

	Render::device.SetBlendMode(Render::ALPHA);

	Render::BeginColor(Color(0, 255, 255, 100));
	size_t count = _chipForChange.size();
	for(size_t i = 0; i < count;i++)
	{
		IPoint ipos = _chipForChange[i].pos;
		ipos = ipos*GameSettings::SQUARE_SIDE;

		MyAssert(_chipForChange[i].color >=0);

		Render::DrawFrame(IRect(ipos.x, ipos.y, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE).Inflated(-_chipForChange[i].color*2));
	}
	Render::EndColor();

	Render::device.SetTexturing(true);

	//Рисуем номера групп в изолированых (отрезанных одной клетках) множествах
	//Тонкие места не нумеруются они = UINT_MAX
	Render::FreeType::BindFont("debug");
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			MaybeCell &cell = _maybeMoveCell[x][y];
			if(!cell.exist)
			{
				continue;
			}
			FPoint pos = start_offset + IPoint(x,y);
			pos = pos*GameSettings::SQUARE_SIDE;
			pos += GameSettings::CELL_HALF;
		}
	}
}

void MaybeMoveHint::Clear()
{
	_chipForChange.clear();
}


bool MaybeMoveHint::FindForColor(int current_color)
{
	//1. Обнуляем

	for(size_t x = 0; x < 20;x++)
	{
		for(size_t y = 0; y < 20;y++)
		{
			_maybeMoveCell[x][y] = MaybeCell();
		}
	}
	//2. Составляем бинарную карту 
	IPoint start_offset = Game::activeRect.LeftBottom();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress a = Game::FieldAddress(start_offset.x + x, start_offset.y + y);
			Game::Square *sq = GameSettings::gamefield[a];
			if(Game::isVisible(sq) && !sq->IsHardStand() && sq->GetChip().IsExist() && (sq->GetChip().IsColor(current_color) || sq->GetChip().IsAdapt()))
			{
				_maybeMoveCell[x][y].exist = true;
			}
			_maybeMoveCell[x][y].ice = sq->IsIce();
		}
	}

	// 3. Строим связи
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			MaybeCell &cell = _maybeMoveCell[x][y];
			if(!cell.exist)
			{
				continue;
			}

			for(size_t k = 0; k < Gadgets::checkDirsChains.count; k++)
			{
				int next_x = x + Gadgets::checkDirsChains.dx[k];
				if(next_x < 0 || next_x >= 20){
					continue; // Выход за пределы массива _maybeMoveCell
				}
				int next_y = y + Gadgets::checkDirsChains.dy[k];
				if(next_y < 0 || next_y >= 20){
					continue; // Выход за пределы массива _maybeMoveCell
				}
				if( _maybeMoveCell[next_x][next_y].exist)
				{
					cell.dirs[k] = true;
					cell.dirs_count++;
					if(cell.dirs_count >= 2)
					{
						//Можно сделать ход
						return true;
					}
				}
			}
		}
	}
	return false; //Сделать ход не получится нужен решафл.
}

bool MaybeMoveHint::Find()
{
	if(!_needUpdate)
	{
		return false;
	}
	Clear();
	_chainsExist = false;

	size_t count_colors = Gadgets::levelColors.GetCountFull();
	for(size_t i = 0; i < count_colors; i++)
	{
		if(FindForColor(Gadgets::levelColors[i]))
		{
			_chainsExist = true;
			break;
		}
	}
	_needUpdate = false;
	if(_chainsExist)
	{
		return true;
	}

	//Ничего не составляется, составляем карту возможных замен
	FindChipsForRecolor();

	return true;
}

void MaybeMoveHint::NeedUpdate()
{
	_needUpdate = true;
}

bool MaybeMoveHint::IsEmpty() const
{
	return !_chainsExist;
}

bool MaybeMoveHint::FindChipsForRecolor()
{
	/*Будем использовать переменную checked в кеше данных для обозначения цвета, -1 - нет фишки*/
	//1. Обнуляем
	_chipForChange.clear();

	for(size_t x = 0; x < 20; x++)
	{
		for(size_t y = 0; y < 20; y++)
		{
			_maybeMoveCell[x][y] = MaybeCell();
		}
	}
	//2. Составляем бинарную карту 
	IPoint start_offset = Game::activeRect.LeftBottom();
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			Game::FieldAddress a = Game::FieldAddress(start_offset.x + x, start_offset.y + y);
			Game::Square *sq = GameSettings::gamefield[a];
			if(Game::isVisible(sq) && !sq->IsHardStand() && sq->GetChip().IsExist())
			{
				_maybeMoveCell[x][y].checked = sq->GetChip().GetColor();
			}
			_maybeMoveCell[x][y].ice = sq->IsIce();
		}
	}
	//3. Составляем карту наличия фишек и направлений
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			MaybeCell &cell = _maybeMoveCell[x][y];
			if(cell.checked < 0)
			{
				continue;
			}
			for(size_t k = 0; k < Gadgets::checkDirsChains.count; k++)
			{
				int next_x = x + Gadgets::checkDirsChains.dx[k];
				if(next_x < 0 || next_x >= 20){
					continue; //Выход за пределы
				}
				int next_y = y + Gadgets::checkDirsChains.dy[k];
				if(next_y < 0 || next_y >= 20){
					continue; //Выход за пределы
				}
				if( _maybeMoveCell[next_x][next_y].checked >= 0)
				{
					cell.dirs[k] = true;
					cell.dirs_count++;
				}
			}
		}
	}
	//4. Ищем все фишки в окрестностях 2 фишек
	for(int x = 0; x < Game::activeRect.Width();x++)
	{
		for(int y = 0; y < Game::activeRect.Height();y++)
		{
			MaybeCell &cell = _maybeMoveCell[x][y];
			if(cell.checked < 0 || cell.dirs_count < 2) //Нечего проверять фишки у которых меньше двух соседей. Ну перекрасим, а ход все равно не составить
			{
				continue;
			}
			for(size_t k1 = 0; k1 < Gadgets::checkDirsChains.count;k1++)
			{
				if(!cell.dirs[k1])
				{
					continue;
				}
				IPoint p1 = IPoint(x, y) + Gadgets::checkDirsChains[k1];
				MaybeCell &cell_1 = _maybeMoveCell[p1.x][p1.y];
				//Берем цвет и сравниваем его со следующими в обходе против часовой стрелки
				for(size_t k2 = k1+1; k2 < Gadgets::checkDirsChains.count; k2++)
				{
					if(!cell.dirs[k2])
					{
						continue;
					}
					IPoint p2 = IPoint(x,y) + Gadgets::checkDirsChains[k2];
					MaybeCell &cell_2 = _maybeMoveCell[p2.x][p2.y];
					if( !cell.ice || !cell_2.ice )
					{
						if(cell_2.checked == cell_1.checked)
						{
							//Нашли две фишки разделенные друг от друга одной фишкой
							_chipForChange.push_back(ChipForChange());
							_chipForChange.back().pos = IPoint(x,y) + start_offset;
							_chipForChange.back().color = cell_1.checked;
						}
						else if(cell_1.checked == cell.checked)
						{
							//Если цвета первых двух фишек сопадают - то можем перекрашивать третью
							_chipForChange.push_back(ChipForChange());
							_chipForChange.back().pos = p2 + start_offset;
							_chipForChange.back().color = cell_1.checked;
						}
					}
				}
			}
		}
	}
	if(_chipForChange.empty())
	{
		//5. По одной фишке заменить не удалось. Пробуем заменить по две.
		//Вариантов должно быть не очень много
		for(int x = 0; x < Game::activeRect.Width();x++)
		{
			for(int y = 0; y < Game::activeRect.Height();y++)
			{
				MaybeCell &cell = _maybeMoveCell[x][y];
				if(cell.checked < 0 || cell.dirs_count < 2) //Нечего проверять фишки у которых меньше двух соседей. Ну перекрасим, а ход все равно не составить
				{
					continue;
				}
				for(size_t k1 = 0; k1 < Gadgets::checkDirsChains.count; k1++)
				{
					if(!cell.dirs[k1])
					{
						continue;
					}
					IPoint p1 = IPoint(x,y) + Gadgets::checkDirsChains[k1];
					MaybeCell &cell_1 = _maybeMoveCell[p1.x][p1.y];
					if(cell_1.checked == cell.checked)
					{
						continue; //Цвета совпадают и так. Пропускаем.
					}
					//Берем цвет и сравниваем его со следующими в обходе против часовой стрелки
					for(size_t k2 = k1+1; k2 < Gadgets::checkDirsChains.count; k2++)
					{
						if(!cell.dirs[k2])
						{
							continue;
						}
						IPoint p2 = IPoint(x, y) + Gadgets::checkDirsChains[k2];
						MaybeCell &cell_2 = _maybeMoveCell[p2.x][p2.y];

						if( !cell.ice || !cell_1.ice || !cell_2.ice )
						{
							_chipForChange.push_back(ChipForChange());
							_chipForChange.back().pos = p1 + start_offset;
							_chipForChange.back().color = cell.checked;
							_chipForChange.back().pos2 = p2 + start_offset;
							_chipForChange.back().color2 = cell.checked;
						}
					}
				}
			}
		}
	}
	return !_chipForChange.empty();
}

ChipForChange MaybeMoveHint::GetChipForChange()
{
	if(_chipForChange.empty())
	{
		return ChipForChange();
	}
	return _chipForChange[math::random(0u, _chipForChange.size()-1)];
}