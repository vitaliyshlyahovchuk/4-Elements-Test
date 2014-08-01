#include "stdafx.h"
#include "PictureGenerator.h"

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
#include "Utils/Xml.h"

namespace Utils2D
{

void DrawField()
{
	//if(FieldStyles::current->fon_static_texture)
	//{
	//	Render::device.PushMatrix();
	//	Render::device.MatrixTranslate( math::Vector3(0.0f, 0.0f, ZBuf::BACKGROUND) );
	//	FieldStyles::current->fon_static_texture->Draw(0.f, 0.f, Render::device.Width(), Render::device.Height(), FRect(0.f, 1.f, 0.f, 1.f));
	//	Render::device.PopMatrix();
	//}


	////Подготавливаем массивы летающих клеток
	//std::list<Game::Square*> flying_squares_up, flying_squares_down;
	//for(auto sq: GameSettings::squares)
	//{
	//	if(sq->IsFlyType(Game::Square::FLY_STAY))
	//	{
	//		continue;
	//	}
	//	if(sq->_flySquareOffset.y < 0)
	//	{
	//		flying_squares_down.push_back(sq);
	//	}else{
	//		flying_squares_up.push_back(sq);
	//	}
	//}
	//flying_squares_down.sort(Game::Square::FlySort);
	//flying_squares_up.sort(Game::Square::FlySort);

	Render::device.SetDepthTest(true);
	Render::device.SetDepthWrite(true);
	/************************************************************
	 Отрисовка с включенным Z-тестом и записью в буфер глубины
	************************************************************/
	// Здесь рисуем только полностью непрозрачные объекты, в произвольном
	// порядке (но лучше сортировать от ближних к дальним)

	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(-GameSettings::fieldX, -GameSettings::fieldY, 0.0f));

	Render::device.SetStencilTest(true);
	GameField::StartStencilWrite(0xA0, 0xF0);
	Gadgets::wallDrawer.DrawStone();
	GameField::StartStencilWrite(0xB0, 0xF0);
	Gadgets::wallDrawer.DrawIce();
	Render::device.SetStencilTest(false);

	Gadgets::wallDrawer.DrawWood();
	Gadgets::wallDrawer.DrawGround();
	
	GameField::Get()->DrawEnergy();

	Gadgets::wallDrawer.DrawFieldBase();

	//for(auto sq :flying_squares_down)
	//{
	//	sq->DrawFly();
	//}
	Render::device.PopMatrix();

	//DrawBackground();
	
	Render::device.SetDepthWrite(false);
	/************************************************************
	 Отрисовка с включенным Z-тестом без записи в буфер глубины
	************************************************************/

	Render::device.PushMatrix();
	{
		Render::device.MatrixTranslate(math::Vector3(-GameSettings::fieldX, -GameSettings::fieldY, 0.0f));

		Render::device.SetStencilTest(true);
		GameField::StartStencilTest(Render::StencilFunc::EQUAL, 0xC0, 0xF0);
		Gadgets::bears.Draw(true);
		Energy::field.DrawParticles();
		GameField::StartStencilTest(Render::StencilFunc::NOTEQUAL, 0xC0, 0xF0);
		Gadgets::bears.Draw(false);
		Render::device.SetStencilTest(false);
	

		Gadgets::receivers.DrawDown(); //Часть ресивера рисуется между подложкой и энергией

		Gadgets::wallDrawer.DrawGroundTransparent();
		Gadgets::wallDrawer.DrawGroundBorders();
		Gadgets::wallDrawer.DrawWoodBorders();
		//levelBorder.Draw();

		// отрисовываем те части клеток, которые находятся за стенами и льдом
		Render::device.PushMatrix();
		Render::device.MatrixTranslate( math::Vector3(0.0f, 0.0f, ZBuf::CHIPS) );
		Gadgets::receivers.DrawBase();
		//DrawSquares();
		Render::device.PopMatrix();

	
		Render::device.SetStencilTest(true);
		GameField::StartStencilTest(Render::StencilFunc::NOTEQUAL, 0xA0, 0xF0); // границы льда не будут рисоваться поверх стен...
		Gadgets::wallDrawer.DrawIceBorders();
		GameField::StartStencilTest(Render::StencilFunc::NOTEQUAL, 0xB0, 0xF0); // ...а границы стен поверх льда
		Gadgets::wallDrawer.DrawStoneBorders();
		Render::device.SetStencilTest(false);



		Render::device.SetDepthTest(false);
		/************************************************************
				 Отрисовка без использования буфера глубины
		************************************************************/

	Gadgets::bears.DrawOverlay();

	//DrawSequenceAffectedZone(); // подсветка области поражения

		//if(!_chipSeq.empty()) {
		//	// выделение текущей цепочки
		//	_selectionChip.Draw(_chipSeq, _localTime);
		//}

	
	//if(EditorUtils::editor){
	//	DrawEditorChips();
	//	Gadgets::lockBarriers.Draw();
	//}
	//_effUnderChipsCont.DrawBlend();
	//	{
	//		// риcуем эффекты
	//		for (int k = 1; k <= 3; k++)
	//		{
	//			for(GameFieldController *c : _controllers)
	//			{
	//				if (c->z == k)
	//					c->DrawUnderChips();
	//		}
	//
	//	}
	//}

		Gadgets::snapGadgets.DrawGame();
		//DrawChips(0); // обычные фишки, колышки, монстрики
		Gadgets::gates.Draw();
		//Gadgets::receivers.DrawUp();
		Gadgets::lockBarriers.DrawUp();
		//DrawChips(1); // фишки с бонусами
		//DrawChips(2); // летящие и падающие фишки

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
	
	
		//for(auto sq: flying_squares_up)
		//{
		//	sq->DrawFly();
		//}
	

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
		//_effCont.DrawBlend();

		//Gadgets::snapGadgets.DrawEdit();
		
		//рисуем бонус
	
		//if (BoostList::HardPtr currentActiveBoost = _currentActiveBoost.lock())
		//{
		//	currentActiveBoost->Draw();
		//}

		// риcуем эффекты
		//for (int k = 1; k <= 3; k++)
		//{
		//	for (GameFieldControllerList::iterator ic = _controllers.begin(); ic != _controllers.end(); ++ic)
		//	{
		//		if ((*ic)->z == k)
		//			(*ic)->Draw();
		//	}
		//}

		//
		//_effTopCont.DrawBlend();

		//if(EditorUtils::draw_debug)
		//{
		//	DetectBorder2D::Draw();
		//	Gadgets::freeFrontDetector.DrawEdit();
		//}

		//_fuuuTester.Draw();
	}
	Render::device.PopMatrix();
	
	// включим запись в буфер глубины, чтобы он мог очиститься в начале следующего кадра
	Render::device.SetDepthWrite(true);	
}

	PictureCellGenerator::PictureCellGenerator()
	{
		//Подготовка
		Render::Texture *saved_tex = FieldStyles::current->fon_static_texture;
		FieldStyles::current->fon_static_texture = NULL;
		IPoint start_indexes(11, 10), finish_indexes(13, 16);
		IPoint size_field = finish_indexes - start_indexes + IPoint(1,1);
		int width = (size_field.x + 1)*GameSettings::SQUARE_SIDE;
		int height = (size_field.y+1)*GameSettings::SQUARE_SIDE;

		FPoint save_field_pos = FPoint(GameSettings::fieldX, GameSettings::fieldY);
		GameSettings::fieldX = (start_indexes.x - 0.5f)*GameSettings::SQUARE_SIDEF;
		GameSettings::fieldY = (start_indexes.y - 0.5f)*GameSettings::SQUARE_SIDEF;
		GameField::Get()->KillAllEffects();

		Render::Target * target = Render::device.CreateRenderTarget(width, height, true, true, true, Render::MULTISAMPLE_NONE, true);

		Render::device.BeginRenderTo(target, Color::BLACK_TRANSPARENT);
		DrawField(); //Это большой копипаст метода GameField::DrawField
		Render::device.EndRenderTo();
		target->SaveAsPng("textures/Fields/Island/Collection.png");

		Render::device.DeleteRenderTarget(target);

		//Восстановление
		FieldStyles::current->fon_static_texture = saved_tex;
		GameSettings::fieldX = save_field_pos.x;
		GameSettings::fieldY = save_field_pos.y;

		Xml::TiXmlDocument doc;
		Xml::TiXmlElement xml_root_ref("root");
		doc.InsertEndChild(xml_root_ref);
		Xml::TiXmlElement *xml_root = doc.RootElement();
		Xml::TiXmlElement *xml_squares = xml_root->InsertEndChild(Xml::TiXmlElement("List")) -> ToElement();
		for(int x = 0; x <= size_field.x; x+=2)
		{
			for(int y = 0; y <=  size_field.y; y+=2)
			{
				Game::Square *sq = GameSettings::gamefield[start_indexes + IPoint(x, y)];
				if(Game::isVisible(sq))
				{
					Xml::TiXmlElement *xml_cell = xml_squares -> InsertEndChild(Xml::TiXmlElement("Cell")) -> ToElement();
					xml_cell->SetAttribute("mask_id", sq->MakeMask());
					FPoint start_p = FPoint(x*GameSettings::SQUARE_SIDEF/float(width), y*GameSettings::SQUARE_SIDEF/float(height));
					FPoint end_p = FPoint((x + 2.f)*GameSettings::SQUARE_SIDEF/float(width), (y+2.f)*GameSettings::SQUARE_SIDEF/float(height));

					xml_cell->SetAttribute("xStart", FloatToString(start_p.x, 5));
					xml_cell->SetAttribute("xEnd", FloatToString(end_p.x, 5));
					xml_cell->SetAttribute("yStart", FloatToString(start_p.y, 5));
					xml_cell->SetAttribute("yEnd", FloatToString(end_p.y, 5));
				}
			}
		}
		doc.SaveFile("IslandCollection.xml");
	}
	
	std::map<int, FRect> PictureCellGenerator::sq_mask_to_frect;
	Render::Texture *PictureCellGenerator::collectionTexture = 0;

	void PictureCellGenerator::LoadSettings()
	{
		PictureCellGenerator::sq_mask_to_frect.clear();
		Xml::TiXmlDocument doc;
		if(!doc.LoadFile("IslandCollection.xml"))
		{
			return;
		}
		Xml::TiXmlElement *xml_cell = doc.RootElement()->FirstChildElement("List")->FirstChildElement("Cell");
		while(xml_cell)
		{
			PictureCellGenerator::sq_mask_to_frect[utils::lexical_cast<int>(xml_cell->Attribute("mask_id"))] = FRect(xml_cell);
			xml_cell = xml_cell->NextSiblingElement("Cell");
		}
		PictureCellGenerator::collectionTexture = Core::resourceManager.Get<Render::Texture>("IslandCollection");
	}
}

