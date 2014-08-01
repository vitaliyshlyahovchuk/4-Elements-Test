#include "stdafx.h"
#include "FreeFront.h"
#include "Match3Gadgets.h"
#include "EditorUtils.h"
#include "GameField.h"
#include "SnapGadgetsClass.h"
#include "SquareNewInfo.h"

namespace Gadgets
{
	FreeFrontDetector freeFrontDetector;
	float minFreeDistance = 0.f;

	Array2D<float> squareDistRec;
	Array2D<float> squareDist;

	void FreeFrontInit()
	{
		squareDistRec.Init( GameSettings::FIELD_MAX_WIDTH + 2, GameSettings::FIELD_MAX_HEIGHT + 2, 0.f, 1, 1);	
		squareDist.Init(GameSettings::FIELD_MAX_WIDTH, GameSettings::FIELD_MAX_HEIGHT, 0.f, 0, 0);	
	}

} //namespace Gadgets

Game::FieldAddress ExhaustiveRoundWayElement::GetChildAddres()
{
	return a_checking;
}

void ExhaustiveRoundWayElement::Init(const Game::FieldAddress &new_a, const size_t &new_dir, const bool &use_current_addres)
{
	size_t dir = (new_dir + Gadgets::checkDirsInfo.count) % Gadgets::checkDirsInfo.count;
	if(use_current_addres)
	{
		//Для порталов. У портала все стороны новые, поэтому адрес используется тот же.
		a_checking = new_a;
	}else{
		//Для обычных ячеек
		a_checking = new_a + Game::FieldAddress(Gadgets::checkDirsInfo.dx[dir], Gadgets::checkDirsInfo.dy[dir]);
		GameSettings::gamefield[new_a]->SetEnergyChecked(true);
	}
	debug_dir = dir;

	Game::Square *sq = GameSettings::gamefield[a_checking];
	border = !Game::isVisible(a_checking) || sq->IsEnergyChecked(true);
	if(!border)
	{
		float squareDist_curent = Gadgets::squareDist[a_checking.GetCol()][a_checking.GetRow()];			
		if(Gadgets::minFreeDistance > squareDist_curent)
		{
			//Assert(squareDist_curent >= 0);
			Gadgets::minFreeDistance = squareDist_curent;
		} 
	}
}


void ExhaustiveRoundWayElement::DrawEdit()
{
	FPoint pos = GameSettings::CELL_HALF + a_checking.ToPoint()*GameSettings::SQUARE_SIDE;
	float d_angle = math::PI*0.25f;
	if(Gadgets::checkDirsInfo.count == 8)
	{
		d_angle *= 0.5f;
	}
	FPoint dir_pos(0.f - Gadgets::checkDirsInfo.dx[debug_dir], 0.f - Gadgets::checkDirsInfo.dy[debug_dir]);
	dir_pos*= 27;
	FPoint dir_pos2 = dir_pos.Normalized();
	Render::DrawLine(pos + dir_pos - dir_pos2*5, pos  + dir_pos + dir_pos2*10);
	pos.y += 1.f;
	Render::DrawLine(pos + dir_pos - dir_pos2*5, pos  + dir_pos + dir_pos2*10);
}

//FreeFrontDetector

void FreeFrontDetector::LoadLevel()
{
	_cameraIsStand = true;
	_freeFrontForEnergy.clear();

	Gadgets::minFreeDistance = 100000000000.f;
	
	std::vector<IPoint> vec;
	Gadgets::square_new_info.EnergySource_Get(vec);
	if(!vec.empty())
	{
		for(size_t i = 0; i < vec.size(); i++)
		{
			for(size_t k = 0; k < Gadgets::checkDirsInfo.count; k++)
			{
				_freeFrontForEnergy.push_back(ExhaustiveRoundWayElement());
				_freeFrontForEnergy.back().Init(Game::FieldAddress(vec[i]), k, false);
			}
		}
	}
}

void FreeFrontDetector::DrawEdit()
{
	Render::device.SetTexturing(false);
	Render::BeginColor(Color(0, 0, 0));
	size_t count = _freeFrontForEnergy.size();
	for(size_t i = 0; i < count; i++)
	{
		_freeFrontForEnergy[i].DrawEdit();
	}
	Render::EndColor();
	Render::device.SetTexturing(true);
}

void FreeFrontDetector::GrowFront(ExhaustiveRoundWayElement &element, const bool &use_current_addres)
{
	element.border = !use_current_addres;
	for(size_t kk = 0; kk < Gadgets::checkDirsInfo.count; kk++)
	{
		ExhaustiveRoundWayElement next_item;
		//next_item.Init(element.GetChildAddres(), element.dir + kk);
		next_item.Init(element.GetChildAddres(), kk, use_current_addres);
		if(!next_item.border)
		{
			_freeFrontForEnergy.push_back(next_item);
		}
	}
}

void FreeFrontDetector::Update()
{
	//Продвигаем фронт, на столько сколько возможно
	for(size_t i = 0; i < _freeFrontForEnergy.size();i++)
	{
		if(_freeFrontForEnergy[i].border)
		{
			continue;
		}
		Game::FieldAddress child_a = _freeFrontForEnergy[i].GetChildAddres();
		Game::Square *sq = GameSettings::gamefield[child_a]; 
		if(sq->IsEnergyChecked(true))
		{
			_freeFrontForEnergy[i].border = true;
			continue;
		}
		bool can_energy = sq->IsGoodForFreeFront();
		if(can_energy)
		{
			//Меняем одно направление на восемь
			GrowFront(_freeFrontForEnergy[i], false);

			//Обрабатываем порталы
			ExhaustiveRoundWayElement element = _freeFrontForEnergy[i];
				
			std::vector<Game::Square*> vec;
			GameSettings::GetOtherPortalsSquares(child_a, vec);
			size_t count = vec.size();
			for(size_t i = 0; i < count; i++)
			{
				element.a_checking = vec[i]->address;
				//element.dir = i;
				GrowFront(element, true);
			}
		}
	}
	std::vector<ExhaustiveRoundWayElement>::iterator iter = _freeFrontForEnergy.begin();
	while(iter != _freeFrontForEnergy.end())
	{
		if(iter->border)
		{
			iter = _freeFrontForEnergy.erase(iter);
		}else{
			iter++;
		}
	}
	_cameraIsStand = true;
	const SnapGadget *snap = Gadgets::snapGadgets.GetActiveGadget();
	if(snap)
	{
		_cameraIsStand = !snap->IsCheckCameraForRelease(true);
	}

}

bool FreeFrontDetector::CameraIsStand()
{
	return _cameraIsStand;
}

void FreeFrontDetector::SetCameraIsStand()
{
	_cameraIsStand = true;
}
