#include "stdafx.h"
#include "EditorWidget.h"
#include "EditorUtils.h"
#include "GameInfo.h"
#include "Match3Gadgets.h"
#include "GameField.h"
#include "ActCounter.h"
#include "EditorPanel.h"
#include "Match3Border.h"
#include "FieldStyles.h"
#include "FieldBears.h"
#include "SnapGadgetsClass.h"
#include "EnergyReceivers.h"
#include "ChangeEnergySpeedClass.h"
#include "BombField.h"
#include "RyushkiGadget.h"
#include "SquareNewInfo.h"

const IRect EDIT_ARROW_RECT = IRect(10, 100, 96, 96);

namespace EditorUtils
{
	EditorWidget::EditorWidget(const std::string& name_, rapidxml::xml_node<>* elem_)
		: GUI::Widget(name_, elem_)		
		, _localTime(0.f)
		, _wallColorBrush(0)
		, _mouseDown(false)
		, _selectedOrder(NULL)
	{
		//EditorPanel::offset_panel.x = gameInfo.getInt("EditorPanel::offset_panel", 0);
		chipsTex = Core::resourceManager.Get<Render::Texture>("Chips");
	}

	EditorWidget::~EditorWidget()
	{
	
	}

	void EditorWidget::Update(float dt)
	{
		_localTime += dt;
		if (EditorUtils::editor)
		{
			EditorUtils::editorToolTip.alpha -= dt;
		}
		Gadgets::snapGadgets.Update(dt);
	}

	void EditorWidget::Editor_DrawColoredWallTool()
	{
		if(EditorUtils::activeEditBtn == EditorUtils::WallColored)
		{
			FRect uv = Game::GetChipRect(_wallColorBrush, false, false, false);
			Color color = GameSettings::chip_settings[_wallColorBrush].color_wall;
			IRect rect = Game::ChipColor::DRAW_RECT.MovedBy(IPoint(10,10));

			Render::device.SetTexturing(false);
			Render::BeginColor(color);
			Render::DrawRect(rect);
			Render::EndColor();
			Render::device.SetTexturing(true);
			chipsTex->Draw(rect, uv);
		}
	}

	void EditorWidget::Draw()
	{
		IPoint mouse_pos = Core::mainInput.GetMousePos();

		Render::device.PushMatrix();
		Render::device.MatrixTranslate(GameSettings::FIELD_SCREEN_OFFSET);
		{
			Editor_DrawColoredWallTool();
			Render::device.PushMatrix();//Translate fieldX fieldY
			Render::device.MatrixTranslate(math::Vector3(-GameSettings::fieldX, -GameSettings::fieldY, 0.0f));
			{
				//Рисуем гаджеты
				//Gadgets::snapGadgets.Editor_Draw();
				Editor_DrawFieldLattice();
				Editor_DrawMoveTool();
				Editor_DrawFieldRadar();
				Gadgets::energySpeedChangers.Editor_Draw();

				for(size_t i = 0; i < Gadgets::editor_makers.size();i++)
				{
					Gadgets::editor_makers[i]->DrawEdit();
				}
				if(GameSettings::underMouseIndex.x >= 0 && GameSettings::underMouseIndex.y >=0 )
				{
					Render::device.SetTexturing(false);
					Render::device.SetBlendMode(Render::ADD);
					Render::BeginColor(Color(255, 255,255, 50));
					IRect rect(GameSettings::underMouseIndex.x*GameSettings::SQUARE_SIDE, GameSettings::underMouseIndex.y*GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);
					for(BYTE i = 0; i < 2; i++)
					{
						rect.Inflate(1);
						Render::DrawFrame(rect); 
					}
					Render::device.SetBlendMode(Render::ALPHA);
					Render::device.SetTexturing(true);
					Render::EndColor();
				}
				if (EditorUtils::activeEditBtn == EditorUtils::RyushkiAddEdit)
				{
					Gadgets::ryushki.Editor_DrawField();
				}
			}
			//Translate fieldX fieldY
			Render::device.PopMatrix();

			Gadgets::DrawEdit();
		}
		Render::device.PopMatrix();

		//Слайдеры рюшки
		if (EditorUtils::activeEditBtn == EditorUtils::RyushkiAddEdit)
		{
			Gadgets::ryushki.Editor_Draw();
		}
		if(EditorUtils::activeEditBtn == EditorUtils::SnapGadgetAdd)
		{
			Render::device.SetTexturing(false);
			Render::BeginColor(Color(255, 100, 100));
			Render::device.SetBlendMode(Render::ADD);

			IPoint pos_center;
			pos_center.x = GameSettings::FIELD_SCREEN_OFFSET.x + GameSettings::VIEW_RECT.width/2;
			pos_center.y = GameSettings::FIELD_SCREEN_OFFSET.y + GameSettings::VIEW_RECT.height/2;
			Render::DrawLine(pos_center + IPoint(-100, 0), pos_center + IPoint(100, 0));
			Render::DrawLine(pos_center + IPoint(0, -100), pos_center + IPoint(0, 100));

			Render::device.SetBlendMode(Render::ALPHA);
			Render::EndColor();
			Render::device.SetTexturing(true);
		}
	}

	bool EditorWidget::MouseDown(const IPoint &mouse_pos)
	{
		_mouseDown = true;
		if (GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos))
		{
			Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
			_mouseDownPos = index.ToPoint();
			Game::Square* sq = GameSettings::gamefield[index];
			if (EditorUtils::activeEditBtn == EditorUtils::RyushkiAddEdit)
			{
				Gadgets::ryushki.Editor_MouseDown(mouse_pos);
			}
			if (Core::mainInput.GetMouseRightButton())
			{
				for(size_t i = 0; i < Gadgets::editor_makers.size(); i++)
				{
					if( Gadgets::editor_makers[i]->Editor_RightMouseDown(mouse_pos, sq))
					{
						return true;
					}
				}

				//Правая кнопка мыши
				if (EditorUtils::activeEditBtn == EditorUtils::EnergySpeedField)
				{
					Gadgets::energySpeedChangers.Editor_RemoveUnderMouse(mouse_pos, index.GetCol(), index.GetRow());
					return true;
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::BombField_Add)
				{
					Gadgets::bombFields.Editor_RemoveUnderMouse(mouse_pos, index.GetCol(), index.GetRow());
					return true;
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::EditOrderArea)
				{
					if( (*_selectedOrder)->GetType() == Game::Order::FILL_ENERGY )
					{
						Game::FillEnergyOrder *order = (Game::FillEnergyOrder*)_selectedOrder->get();
						order->DeleteCell(index);
						return true;
					}
				}

				if (Game::isVisible(sq) && (!sq->IsFake()))
				{
					if (sq->ice > 0)
					{
						sq->ice = 0;
						return false;
					}

					if (sq->GetChip().IsMusor())
					{
						sq->GetChip().Reset(true);
						return false;
					}

					if (sq->IsStone())
					{
						sq->SetStone(false);
						return false;
					}
				}

				if (EditorUtils::activeEditBtn == EditorUtils::EditMusor)
				{
					if (sq->GetChip().IsMusor())
					{
						sq->GetChip().Reset(true);
						return true;
					}
				}

				if (Game::isVisible(sq))
				{					
					if(sq->IsFake()) {
						sq->SetFake(false);
					} else if(sq->IsSand()) {
						sq->SetSand(false, true);
					} else if(sq->IsIndestructible()) {
						sq->SetIndestructible(false);
					} else if(sq->IsPermanent()) {
						sq->SetPermanent(false);
					} else if(sq->IsRestoring()) {
						sq->SetRestoring(false);
					} else if(sq->GetWallColor() > 0) {
						sq->SetWallColor(0);
					} else {
						Log::Info("Ground deleted");
						sq->SetWall(sq->GetWall()-1);
						if (sq->GetWall() == -1)
						{
							GameSettings::gamefield[index] = Game::bufSquare;
							GameSettings::recfield[index] = false;

							GameSettings::EraseCell(sq);
						}					
					}
					// В ячейке ведь могло что-то изменитьcя!
					EditorUtils::underMouseEditorSquare = GameSettings::gamefield[index];
				}
			} else {							
				for(size_t i = 0; i < Gadgets::editor_makers.size(); i++)
				{
					if(Gadgets::editor_makers[i]->Editor_LeftMouseDown(mouse_pos, sq))
					{
						return true;
					}
				}
				// В редакторе и нажата левая кнопка мышки...
				if (EditorUtils::activeEditBtn == EditorUtils::EnergySpeedField)
				{
					Gadgets::energySpeedChangers.Editor_MouseDown(mouse_pos);
				}
				else if( EditorUtils::activeEditBtn == EditorUtils::PlaceOrder )
				{
					_selectedOrder = NULL;
					for(size_t i = 0; i < Gadgets::editor_makers.size(); i++)
					{
						_selectedOrder = Gadgets::editor_makers[i]->Editor_SelectOrder(mouse_pos, sq);
						if(_selectedOrder)
						{
							_selectedOrderAddress = sq->address.ToPoint();
							_selectedOrderArea.clear();

							if( !(*_selectedOrder) ) {
								(*_selectedOrder) = boost::make_shared<Game::EmptyOrder>();
							}

							// init order config window
							Layer *layer = Core::guiManager.getLayer("OrderConfig");
							Game::Order::Type type = (*_selectedOrder)->GetType();
							if( type == Game::Order::EMPTY )
							{
								layer->getWidget("OrderType_text")->AcceptMessage(Message("Set", "empty"));
								layer->getWidget("OrderAmount")->AcceptMessage(Message("Set", "0"));
								layer->getWidget("OrderConfig")->AcceptMessage(Message("SetType", "empty"));
							}
							else if( type == Game::Order::KILL_CELL )
							{
								Game::DestroyChipsOrder *order = (Game::DestroyChipsOrder*)_selectedOrder->get();
								layer->getWidget("OrderType_text")->AcceptMessage(Message("Set", order->GetObjectiveType()));
								layer->getWidget("OrderAmount")->AcceptMessage(Message("Set", utils::lexical_cast(order->GetCount())));
								layer->getWidget("OrderConfig")->AcceptMessage(Message("SetType", order->GetObjectiveType()));
								layer->getWidget("OrderConfig")->AcceptMessage(Message("SetColor", order->GetChipColor()));
							}
							else if( type == Game::Order::FILL_ENERGY )
							{
								Game::FillEnergyOrder *order = (Game::FillEnergyOrder*)_selectedOrder->get();
								layer->getWidget("OrderType_text")->AcceptMessage(Message("Set", "fill_energy"));
								layer->getWidget("OrderAmount")->AcceptMessage(Message("Set", "0"));
								layer->getWidget("OrderConfig")->AcceptMessage(Message("SetType", "fill_energy"));
								_selectedOrderArea = order->GetArea();
							}

							Core::mainScreen.pushLayer(layer);
							return true;
						}
					}
					return false;
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::EditOrderArea)
				{
					if( (*_selectedOrder)->GetType() == Game::Order::FILL_ENERGY && Game::isSquare(sq))
					{
						Game::FillEnergyOrder *order = (Game::FillEnergyOrder*)_selectedOrder->get();
						order->AddCell(index);
						if(sq->GetWall() == 0 )
							sq->SetWall(1);
					}
				}

				if ((EditorUtils::activeEditBtn >= EditorUtils::Wall0 && EditorUtils::activeEditBtn <= EditorUtils::Wall3) 
					|| (EditorUtils::activeEditBtn == EditorUtils::InvisibleField) 
					|| (EditorUtils::activeEditBtn == EditorUtils::FakeField)
					|| (EditorUtils::activeEditBtn == EditorUtils::WallSand)
//					|| (EditorUtils::activeEditBtn == EditorUtils::GrowingWoodEdit)
				)
				{
					
					if (!Game::isSquare(sq))
					{
						Game::Square *s = Game::GetValidSquare(index);
						if (EditorUtils::activeEditBtn == EditorUtils::InvisibleField)
						{
							s->invisible = true;
							s->SetFake(true);
							s->SetWall(0);
						}
						else if (EditorUtils::activeEditBtn == EditorUtils::FakeField)
						{
							s->invisible = false;
							s->SetFake(true);
							s->SetWall(0);
							s->SetSand(false, true);
						}
						else if (EditorUtils::activeEditBtn == EditorUtils::WallSand)
						{
							s->invisible = false;
							s->SetFake(false);
							s->SetWall(1);
							s->SetSand(true, true);
						}
						else // нажата одна из кнопок отвечающих за стены
						{
							s->invisible = false;
							s->SetFake(false);
							s->SetWall(static_cast<int>(EditorUtils::activeEditBtn));
							s->SetSand(false, true);
						}
						
						s->ice = 0;
						s->SetStone(false);
						s->GetChip().Reset(true);
						
						GameField::gameField->UpdateGameField();

						GameField::gameField->_levelScopeUpdated = false;
						GameField::gameField->_levelScopeUpdateTime = 0.0f;						
					} 
					else 
					{
						if (sq->barrierIndex == -1)
						{	
							if (EditorUtils::activeEditBtn == EditorUtils::InvisibleField)
							{
								sq->invisible = true;
								sq->SetFake(true);
								sq->SetWall(0);
							}
							else if (EditorUtils::activeEditBtn == EditorUtils::FakeField)
							{
								sq->invisible = false;
								sq->SetFake(true);
								sq->SetWall(0);
							}
							else if (EditorUtils::activeEditBtn == EditorUtils::WallSand)
							{
								sq->SetFake(false);
								sq->invisible = false;
								sq->SetWall(1);
								sq->SetSand(true, true);
							}
							else // нажата одна из кнопок отвечающих за стены
							{
								sq->SetFake(false);
								sq->invisible = false;
								sq->SetWall(static_cast<int>(EditorUtils::activeEditBtn));
								sq->SetSand(false, true);
								sq->SetIndestructible(false);
								sq->SetPermanent(false);
								sq->SetWallColor(0);
								sq->SetRestoring(false);
							}
							
							sq->ice = 0;
							sq->SetStone(false);
							sq->GetChip().SetMusor(false);
						}
					}
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::WallIndestructible)
				{
					if( sq->GetWall() > 0 )
						sq->SetIndestructible(true);
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::WallPermanent)
				{
					if( sq->GetWall() > 0 )
						sq->SetPermanent(true);
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::WallColored)
				{
					if( sq->GetWall() > 0 ) {
						sq->SetWallColor(_wallColorBrush);
					}
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::WallRestoring)
				{
					if( sq->GetWall() > 0)
						sq->SetRestoring(true);
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::EditMusor)
				{
					if (!Game::isVisible(sq))
					{
						Game::Square *s = Game::GetValidSquare(index);

						s->GetChip().Reset(true);
						s->SetWall(0);
						s->ice = 0;
						s->SetStone(false);

						s->GetChip().SetMusor(true);

						GameField::gameField->UpdateGameField();

						GameField::gameField->_levelScopeUpdated = false;
						GameField::gameField->_levelScopeUpdateTime = 0.0f;
					} 
					else 
					{
						if (!Gadgets::Editor_RemoveMinigameObject(sq, EditorUtils::activeEditBtn))
						{
							sq->SetFake(false);
							sq->SetStone(false);
							sq->GetChip().SetMusor(true);
						}
					}
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::ChipCyclops)
				{
					if (Game::isVisible(sq))
					{
						sq->Reset();
						sq->SetCyclops(true);
						sq->GetChip().SetGroundCyclops();
					}
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::AddIce)
				{
					if (Game::isVisible(sq))
					{
						if (!sq->IsFake() && !sq->GetChip().IsLicorice() && !sq->GetChip().IsThief())
						{
							sq->ice = 1;
							sq->SetStone(false);
						}
					}
				} else if (EditorUtils::activeEditBtn == EditorUtils::AddStone)
				{
					// Удаляем что-либо, еcли еcть...
					Gadgets::Editor_RemoveMinigameObject(sq, EditorUtils::activeEditBtn);

					if (Game::isVisible(sq) && !sq->IsFake())
					{
						sq->SetFake(false);
						sq->SetStone(true);
						sq->GetChip().Reset(true);
						sq->SetWall(1);
						sq->ice = 0;
					}
					else 
					{
						Game::Square *s = Game::GetValidSquare(index);

						s->SetFake(false);
						s->SetStone(true);
						s->GetChip().Reset(true);
						s->SetWall(1);
						s->ice = 0;
						
						GameField::gameField->UpdateGameField();

						GameField::gameField->_levelScopeUpdated = false;
						GameField::gameField->_levelScopeUpdateTime = 0.0f;
					}
				} 
				else if (EditorUtils::activeEditBtn == EditorUtils::EditMusor)
				{
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::Null)
				{
					if (Game::isVisible(sq))
					{
						// Удаляем что-либо, еcли еcть...
						Gadgets::Editor_RemoveMinigameObject(sq, EditorUtils::activeEditBtn);
						
						GameSettings::gamefield[index] = Game::bufSquare;
						GameSettings::recfield[index] = false;

						GameSettings::EraseCell(sq);
						
						// В ячейке ведь могло что-то изменитьcя!
						EditorUtils::underMouseEditorSquare = GameSettings::gamefield[index];
					}
					
					return true;
				} 
				else if (EditorUtils::activeEditBtn == EditorUtils::EnergySpeedField)
				{
					bool captured = Gadgets::energySpeedChangers.Editor_CaptureItem(mouse_pos, index.GetCol(), index.GetRow());
					
					if (!captured) // Не попали по cущеcтвующим элементам
					{
						ChangeEnergySpeed *g = new ChangeEnergySpeed();

						// Преобразовываем координаты к координатам на поле
						int mx = index.GetCol();
						int my = index.GetRow();

						// Уcтанавливаем точку центра
						g -> SetPosition(IPoint(mx, my));
						g -> _selected = true;
						
						Gadgets::energySpeedChangers.Reset();
						Gadgets::energySpeedChangers.AddGadget(g);
					}
					
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::BombField_Add)
				{
					bool captured = Gadgets::bombFields.Editor_CaptureBomb(mouse_pos, index.GetCol(), index.GetRow());
					
					if (!captured) // Не попали по cущеcтвующим элементам
					{
						BombField::HardPtr bomb = boost::make_shared<BombField>();

						// Преобразовываем координаты к координатам на поле
						int mx = index.GetCol();
						int my = index.GetRow();

						// Уcтанавливаем точку центра
						bomb->SetPosition(IPoint(mx, my));
						bomb->selected = true;
						
						Gadgets::bombFields.Reset();
						Gadgets::bombFields.AddBomb(bomb);
					}
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::MoveTool)
				{
					EditorUtils::moveToolSet = true;
					EditorUtils::moveToolArea = IRect(index.ToPoint(), 1, 1);
					EditorUtils::moveToolAreaDrop = EditorUtils::moveToolArea;

					EditorUtils::moveToolValue = Editor_MoveCheckIntersection(EditorUtils::moveToolArea, IPoint(0, 0));
				}
			}
		}
		return false;
		//return GameField::gameField->MouseDown(mouse_pos);
	}

	void EditorWidget::MouseMove(const IPoint &mouse_pos)
	{
		if(!GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos))
		{
			return;
		}
		Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
		EditorUtils::underMouseEditorSquare = GameSettings::gamefield[index];
		GameSettings::underMouseIndex = IPoint(index.GetCol(),index.GetRow());
		if (!index.IsValid())
		{
			return;
		}
		Game::Square *s = GameSettings::gamefield[index];

		for(size_t i = 0; i < Gadgets::editor_makers.size(); i++)
		{
			if( Gadgets::editor_makers[i]->Editor_MouseMove(mouse_pos, s) )
			{
				_mouseDownPos = index.ToPoint();
				return;
			}
		}

		{
			switch (EditorUtils::activeEditBtn)
			{
				case EnergySpeedField: 
					{ 
						Gadgets::energySpeedChangers.Editor_MoveItem(mouse_pos, index.GetCol(), index.GetRow()); 			
						break; 
					}
				case BombField_Add: 
					{ 
						Gadgets::bombFields.Editor_MoveBomb(mouse_pos, index.GetCol(), index.GetRow()); 			
						break; 
					}
			}

			if (EditorUtils::activeEditBtn == EditorUtils::EnergySpeedField)
			{
				Gadgets::energySpeedChangers.Editor_MouseMove(mouse_pos);
			}

			if (Core::mainInput.GetMouseRightButton())
			{
				if(Game::FieldAddress(_mouseDownPos) != index)
				{
					if(_mouseDown)
					{
						MouseDown(mouse_pos);
					}
					_mouseDownPos = index.ToPoint();
					return;
				}
			}
			if (EditorUtils::activeEditBtn == EditorUtils::RyushkiAddEdit)
			{
				Gadgets::ryushki.Editor_MouseMove(mouse_pos);
			}

			if (Core::mainInput.GetMouseLeftButton())
			{
				if ((Game::FieldAddress(_mouseDownPos) != index) && 
					((EditorUtils::activeEditBtn >= EditorUtils::Wall0 && EditorUtils::activeEditBtn <= EditorUtils::Null)
					|| (EditorUtils::activeEditBtn == EditorUtils::InvisibleField) 
					|| (EditorUtils::activeEditBtn == EditorUtils::FakeField)
					|| (EditorUtils::activeEditBtn == EditorUtils::WallSand)
					|| (EditorUtils::activeEditBtn == EditorUtils::WallIndestructible)
					|| (EditorUtils::activeEditBtn == EditorUtils::WallPermanent)
					|| (EditorUtils::activeEditBtn == EditorUtils::WallRestoring)
					|| (EditorUtils::activeEditBtn == EditorUtils::EditMusor)
					|| (EditorUtils::activeEditBtn == EditorUtils::AddIce)
					|| (EditorUtils::activeEditBtn == EditorUtils::AddStone)
					|| (EditorUtils::activeEditBtn == EditorUtils::WallColored)
					|| (EditorUtils::activeEditBtn == EditorUtils::EditOrderArea)
					))
				{
					if(_mouseDown)
					{
						MouseDown(mouse_pos);
					}
					_mouseDownPos = index.ToPoint();
					return;
				}
			}
		}
	}

	void EditorWidget::MouseUp(const IPoint &mouse_pos)
	{
		if(!_mouseDown)
		{
			return; //Кнопку нажали не в поле. 
		}
		_mouseDown = false;
		if (EditorUtils::activeEditBtn == EditorUtils::EnergySpeedField)
		{
			Gadgets::energySpeedChangers.Editor_MouseUp(mouse_pos);
		}	
		
		if (EditorUtils::activeEditBtn == EditorUtils::RyushkiAddEdit)
		{
			Gadgets::ryushki.Editor_MouseUp(mouse_pos);
		}

		if (GameSettings::FIELD_SCREEN_CONST.Contains(mouse_pos))
		{
			Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
			
			{
				Game::Square* sq = GameSettings::gamefield[index];
				for(size_t i = 0; i < Gadgets::editor_makers.size(); i++)
				{
					Gadgets::editor_makers[i]->Editor_MouseUp(mouse_pos, sq);
				}
			}
			if (index.IsValid())
			{
				if (EditorUtils::activeEditBtn == EditorUtils::ForbidNewChips)
				{
					Game::Square* sq = GameSettings::gamefield[index];
					if( sq == Game::bufSquare )
						GameSettings::gamefield[index] = Game::bufSquareNoChip;
					else if( sq == Game::bufSquareNoChip )
						GameSettings::gamefield[index] = Game::bufSquareNoLicorice;
					else if( sq == Game::bufSquareNoLicorice )
						GameSettings::gamefield[index] = Game::bufSquareNoTimeBomb;
					else if( sq == Game::bufSquareNoTimeBomb )
						GameSettings::gamefield[index] = Game::bufSquareNoBomb;
					else if( sq == Game::bufSquareNoBomb )
						GameSettings::gamefield[index] = Game::bufSquare;
				}

				if (EditorUtils::activeEditBtn == EditorUtils::EnergySpeedField)
				{
					Gadgets::energySpeedChangers.Editor_ReleaseItem();
				}
				if (EditorUtils::activeEditBtn == EditorUtils::BombField_Add)
				{
					Gadgets::bombFields.Editor_ReleaseBomb();
				}
			}
		}
		GameField::gameField->MouseUp(mouse_pos);
	}

	void EditorWidget::MouseWheel(int delta)
	{
		IPoint mouse_pos = Core::mainInput.GetMousePos();

		Game::Square *sq = NULL;
		Game::FieldAddress index = GameSettings::GetMouseAddress(mouse_pos);
		if (index.IsValid())
		{
			sq = GameSettings::gamefield[index];
		}

		if(EditorUtils::activeEditBtn == EditorUtils::WallColored)
		{
			int d = (delta >= 0) ? 1 : -1;
			_wallColorBrush = (_wallColorBrush + delta + 20) % 20;
			while( Gadgets::levelColors.GetIndex(_wallColorBrush) < 0)
				_wallColorBrush = (_wallColorBrush + d + 20) % 20;
		}

		for(size_t i = 0; i < Gadgets::editor_makers.size();i++)
		{
			if(Gadgets::editor_makers[i]->Editor_MouseWheel(delta, sq))
			{
				return;
			}
		}
	}

	bool LevelObjectiveValid()
	{
		std::string levelObj = Gadgets::levelSettings.getString("LevelObjective", "receivers");
		if( levelObj == "receivers" )
		{
			return Gadgets::receivers.TotalCount() > 0;
		}
		else if( levelObj == "bears" )
		{
			int amount = Gadgets::levelSettings.getInt("LevelObjectiveAmount", 0);
			return (amount == 0) || (Gadgets::bears.BearsCount() >= amount);
		}
		return true;
	}

	void EditorWidget::AcceptMessage(const Message &message)
	{
		for(size_t i = 0; i < Gadgets::editor_makers.size();i++)
		{
			if(Gadgets::editor_makers[i]->AcceptMessage(message))
			{
				return;
			}
		}

		if(message.is("KeyPress"))
		{
			if (EditorUtils::activeEditBtn == EditorUtils::RyushkiAddEdit)
			{	
				Gadgets::ryushki.AcceptMessage(message);
			}
			int key = utils::lexical_cast <int> (message.getData());
			if ( key == 'r' || key == 'R')
			{
				if( !LevelObjectiveValid() ){
					MessageBox(NULL, "Неверно настроены цели уровня!", "Внимание!", MB_OK);
					return;
				}

				gameInfo.setGlobalString("Editor::last_loaded_level", EditorUtils::lastLoadedLevel);
				gameInfo.setGlobalPoint("Editor::last_field_pos", IPoint((int)GameSettings::fieldX, (int)GameSettings::fieldY));
				
				EditorUtils::editor = false;
				if(!EditorUtils::lastLoadedLevel.empty())
				{
					GameField::Get()->SaveLevel(EditorUtils::lastLoadedLevel);
					//Тут же перезагружаем!
					Gadgets::LoadLevelDescription();
					GameField::Get()->LoadLevelAndRun(EditorUtils::lastLoadedLevel);
					Core::LuaDoString("match3Panel:startGame()");
				} else {
					MyAssert(false);
				}
				GameField::gameField->AcceptMessage(Message("RemoveEditor"));
			}
			else if (key == 'z')
			{
				// Этот вызов перезагрузит текстуры для текущего стиля поля. 
				// Нужно для художников, чтобы смотреть без лишних действий
				// как будет выглядеть редактируемый в данный момент стиль
				GameField::Get()->ApplyFieldStyle();
				GameField::Get()->levelBorder.Calculate();
			}
			else if (key == -VK_DELETE)
			{
				if (EditorUtils::activeEditBtn == RyushkiAddEdit) {
					Gadgets::ryushki.DeleteSelectedRyushka();
				}
			}
			else if (key == -69 /* e */)
			{
				if (EditorUtils::activeEditBtn == EditorUtils::BombField_Add)
				{
					int delta = Core::mainInput.IsControlKeyDown() ? -1 : 1;
					Gadgets::bombFields.IncBangRadius(delta);
				}
			}
			else if (key == '[')
			{
				// Изменяем видимость сетки...
				EditorUtils::latticeVisible = !EditorUtils::latticeVisible;

				return;
			}
			else if (key == ']')
			{
				// Изменяем видимость радара...
				EditorUtils::radarVisible = !EditorUtils::radarVisible;

				return;
			}
			else if (key == '\'')
			{
				// Изменяем видимость точек привязки...
				EditorUtils::gadgetsVisible = !EditorUtils::gadgetsVisible;
				return;
			}
			//else if (key == -VK_F4) 
			//{
			//	// Смещаем всё поле в "левый-нижний-угол" в массиве уровня Game::gamefield. 
			//	// Конечно, он сместится и на экране в левый нижний угол. В помощь кнопки: 
			//	// '[' и ']'. Это нужно для оптимизации файла с уровнями, иначе слева и 
			//	// снизу будет много совсем пустого места (забитого точками)

			//	if (EditorUtils::editor)
			//	{
			//		Editor_MoveFieldOptimize();
			//	}
			//}
			else if (key == -VK_RETURN)
			{
				if ((EditorUtils::activeEditBtn == EditorUtils::MoveTool) && (EditorUtils::moveToolSet))
				{
					int dx = EditorUtils::moveToolAreaDrop.x - EditorUtils::moveToolArea.x;
					int dy = EditorUtils::moveToolAreaDrop.y - EditorUtils::moveToolArea.y;

					if ((dx != 0) || (dy != 0))
					{
						Editor_CutToClipboard(EditorUtils::moveToolArea);
						Editor_PasteFromClipboard(EditorUtils::moveToolAreaDrop.LeftBottom());
						GameField::Get()->UpdateGameField(true);
						EditorUtils::moveToolArea = EditorUtils::moveToolAreaDrop;
					}
				}
				else if (EditorUtils::activeEditBtn == EditorUtils::EditOrderArea )
				{
					EditorUtils::activeEditBtn = EditorUtils::PlaceOrder;
				}
			}else{
				GameField::Get()->AcceptMessage(message);
			}
			if ((EditorUtils::activeEditBtn == EditorUtils::MoveTool) && (EditorUtils::moveToolSet))
			{
				switch(key)
				{
				case -VK_UP:
				case -VK_DOWN:
				case -VK_LEFT:
				case -VK_RIGHT:
					if(Editor_PressMoveToolButton(-key))
					{
						return;
					}
					break;
				default:
					break;
				}
			}

			int step = GameSettings::SQUARE_SIDE;
			if( Core::mainInput.IsKeyToggledOn(VK_SCROLL) ){
				step = GameSettings::SQUARE_SIDE / 2;
			}
			const float boundX = GameSettings::FIELD_MAX_WIDTH * GameSettings::SQUARE_SIDE - GameSettings::FIELD_SCREEN_CONST.width;
			const float boundY = GameSettings::FIELD_MAX_HEIGHT * GameSettings::SQUARE_SIDE - GameSettings::FIELD_SCREEN_CONST.height;
			if (key == -VK_RIGHT) {
				GameSettings::fieldX += step;
				if(GameSettings::fieldX >= boundX) {
					if( Editor_CanMoveFieldBy(-8, 0) )
						Editor_MoveFieldBy(-8, 0);
					else
						GameSettings::fieldX -= step;
				}
			} else if (key == -VK_LEFT) {
				GameSettings::fieldX -= step;
				if(GameSettings::fieldX <= 0.0f) {
					if( Editor_CanMoveFieldBy(8, 0) )
						Editor_MoveFieldBy(8, 0);
					else
						GameSettings::fieldX += step;
				}
			} else if (key == -VK_UP) {
				GameSettings::fieldY += step;
				if(GameSettings::fieldY >= boundY) {
					if( Editor_CanMoveFieldBy(0, -8) )
						Editor_MoveFieldBy(0, -8);
					else
						GameSettings::fieldY -= step;
				}
			} else if(key == -VK_DOWN) {
				GameSettings::fieldY -= step;
				if(GameSettings::fieldY <= 0.0f) {
					if( Editor_CanMoveFieldBy(0, 8) )
						Editor_MoveFieldBy(0, 8);
					else
						GameSettings::fieldY += step;
				}
			}
			GameSettings::fieldX = math::clamp(0.0f, boundX, GameSettings::fieldX);
			GameSettings::fieldY = math::clamp(0.0f, boundY, GameSettings::fieldY);
			Game::UpdateVisibleRects();
			return;
		}else if(message.is("OnPushEditor"))
		{
			Gadgets::ryushki.UnSelectRyushka();
			EditorUtils::underMouseEditorSquare = Game::bufSquare;
			_selectedOrder = NULL;
			while( Gadgets::levelColors.GetIndex(_wallColorBrush) < 0)
				_wallColorBrush = (_wallColorBrush + 1) % 16;
		}
		else if (message.is("RyushkaSelected"))
		{
			Message state = Core::guiManager.getLayer("EditorList")->getWidget("List")->QueryState(Message("CurrentItem"));
			std::string  selectedItem = state.getData();

			if (selectedItem == "")
				return;

			Xml::RapidXmlDocument doc("SpisokRyushek.xml");
			rapidxml::xml_node<>* root = doc.first_node();

			rapidxml::xml_node<>* elem = root->first_node();
			while (elem)
			{
				if (selectedItem == std::string(elem->first_attribute("name")->value()))
				{
					Ryushka::HardPtr r = Ryushka::CreateRyushkaFromXml(elem);
					IPoint p;
					p.x = static_cast<int>(GameSettings::fieldX) + GameSettings::FIELD_SCREEN_CENTER.x - r->getVisibleRect().x/2;
					p.y = static_cast<int>(GameSettings::fieldY) + GameSettings::FIELD_SCREEN_CENTER.y - r->getVisibleRect().y/2;

					EditorUtils::activeEditBtn = EditorUtils::RyushkiAddEdit;

					r->setPosition(p);
					r->SetId();
					Gadgets::ryushki.AddRyushka(r);

					Gadgets::ryushki.UnSelectRyushka();
					return;
				}

				elem = elem->next_sibling();
			}
		}
		else if (message.is("SetRyushkaLink"))
		{
			Gadgets::ryushki.AcceptMessage(message);
		}
		else if(message.is("FillECombobox", "Style"))
		{
			FieldStyles::FillCombobox();
		}
		else if(message.is("PlaceOrder"))
		{
			Assert(_selectedOrder);

			std::string type = message.getData();
			int amount = message.variables.getInt("amount");
			bool delayed = message.variables.getBool("delayed");
			
			if( type == "sequence" ) {
				int color = message.variables.getInt("color");
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::Objective(color), amount, true);
			} else if( type == "chips" ) {
				int color = message.variables.getInt("color");
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::Objective(color), amount, false);
			} else if( type == "walls" ) {
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::WALL, amount, false);
			} else if( type == "licorice" ) {
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::LICORICE, amount, false);
			} else if( type == "pirate" ) {
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::MOVING_MONSTER, amount, false);
			} else if( type == "ground") {
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::GROUND, amount, false);
			} else if( type == "musor") {
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::MUSOR, amount, false);
			} else if( type == "time_bomb") {
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::TIME_BOMB, amount, false);
			} else if( type == "bear") {
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::BEAR, amount, false);
			} else if( type == "diamonds") {
				*_selectedOrder = boost::make_shared<Game::DestroyChipsOrder>(Game::Order::DIAMOND, amount, false);
			} else if( type == "fill_energy") {
				*_selectedOrder = boost::make_shared<Game::FillEnergyOrder>(_selectedOrderArea);
				EditorUtils::activeEditBtn = EditorUtils::EditOrderArea;
			} else if( type == "empty") {
				*_selectedOrder = boost::make_shared<Game::EmptyOrder>();
			}
			(*_selectedOrder)->SetAddress(_selectedOrderAddress);
			(*_selectedOrder)->Activate(!delayed);
			Game::Order::TrackOrder(*_selectedOrder);
		}
		GameField::gameField->AcceptMessage(message);
	}

	void EditorWidget::Editor_DrawFieldLattice()
	{
		// Выходим, еcли нельзя или не нужно риcовать
		if (!EditorUtils::editor || !EditorUtils::latticeVisible) return;

		IPoint minVisible, maxVisible;
		GameSettings::GetVisibleArea(minVisible, maxVisible);

		Render::BeginAlphaMul(60.0f / 255.0f);
		Render::device.SetTexturing(false);

		float x0 = float (minVisible.x * GameSettings::SQUARE_SIDE);
		float y0 = float (minVisible.y * GameSettings::SQUARE_SIDE);

		float w = float ((maxVisible.x - minVisible.x + 1) * GameSettings::SQUARE_SIDE);
		float h = float ((maxVisible.y - minVisible.y + 1) * GameSettings::SQUARE_SIDE);

		// Риcуем вертикальные линии...
		for (int i = minVisible.x; i <= maxVisible.x; i++)
		{
			float dx = float (i * GameSettings::SQUARE_SIDE);
			Render::DrawLine(FPoint(dx, y0), FPoint(dx, y0 + h));
		}

		// Риcуем горизонтальные линии...
		for (int j = minVisible.y; j <= maxVisible.y; j++)
		{
			float dy = float (j * GameSettings::SQUARE_SIDE);
			Render::DrawLine(FPoint(x0, dy), FPoint(x0 + w, dy));
		}

		Render::EndAlphaMul();

		Render::BeginAlphaMul(127 / 255.0f);
		Render::FreeType::BindFont("debug");

		for (int i = minVisible.x; i <= maxVisible.x; i++)
		for (int j = minVisible.y; j <= maxVisible.y; j++)
		{
			if ((i % 5 == 0) && (j % 5 == 0))
			{
				int x = i * GameSettings::SQUARE_SIDE + 5;
				int y = j * GameSettings::SQUARE_SIDE + 10;

				// char coords[8] = {};
				// sprintf_s(coords, 8, "%03d:%03d", i, j);
				// Render::PrintString(x, y, std::string(coords));

				std::string  s = utils::lexical_cast(i);
				Render::PrintString(x, y + 15, s);

				s = utils::lexical_cast(j);
				Render::PrintString(x, y, s);
			}
		}

		Render::device.SetTexturing(true);
		Render::EndAlphaMul();
	}

	void EditorWidget::Editor_DrawMoveTool()
	{
		if (!EditorUtils::moveToolSet)
		{
			return;
		}
		if (EditorUtils::activeEditBtn != EditorUtils::MoveTool)
		{
			return;
		}

		Render::device.SetTexturing(false);

		// Риcуем выделенную облаcть
		IRect draw (EditorUtils::moveToolArea);
		draw.height *= GameSettings::SQUARE_SIDE;
		draw.width *= GameSettings::SQUARE_SIDE;
		draw.x *= GameSettings::SQUARE_SIDE;
		draw.y *= GameSettings::SQUARE_SIDE;

		Render::BeginColor(Color(255, 0, 0));
		Render::DrawFrame(draw);
		Render::EndColor();
		
		Render::BeginColor(Color(255, 0, 0, 64));
		Render::DrawRect(draw);
		draw.Inflate(-1);
		Render::DrawFrame(draw);
		draw.Inflate(2);
		Render::DrawFrame(draw);
		Render::EndColor();

		// Риcуем облаcть, куда двигаем
		draw = EditorUtils::moveToolAreaDrop;
		draw.height *= GameSettings::SQUARE_SIDE;
		draw.width *= GameSettings::SQUARE_SIDE;
		draw.x *= GameSettings::SQUARE_SIDE;
		draw.y *= GameSettings::SQUARE_SIDE;

		Render::BeginColor(Color(0, 255, 0));
		Render::DrawFrame(draw);
		Render::EndColor();
		
		Render::BeginColor(Color(0, 255, 0, 64));
		Render::DrawRect(draw);
		draw.Inflate(-1);
		Render::DrawFrame(draw);
		draw.Inflate(2);
		Render::DrawFrame(draw);
		Render::EndColor();

		Render::device.SetTexturing(true);

		// Риcуем текcт!
		std::string  str = "no";
		if (EditorUtils::moveToolValue == 0) str = "yes";


		int x = EditorUtils::moveToolArea.x * GameSettings::SQUARE_SIDE;
		int y = EditorUtils::moveToolArea.y * GameSettings::SQUARE_SIDE;

		Render::FreeType::BindFont("debug");
		Render::BeginColor(Color(255, 0, 0, 191));
		Render::PrintString(IPoint(x + 4, y + 1), "cut", 1.f, LeftAlign, BottomAlign);

		x += EditorUtils::moveToolArea.width * GameSettings::SQUARE_SIDE;
		Render::PrintString(IPoint(x - 3, y + 1), str, 1.0f, RightAlign, BottomAlign);
		Render::EndColor();

		// Риcуем текcт 2!
		x = EditorUtils::moveToolAreaDrop.x * GameSettings::SQUARE_SIDE;
		y = EditorUtils::moveToolAreaDrop.y;
		y += EditorUtils::moveToolAreaDrop.height;
		y = y * GameSettings::SQUARE_SIDE - Render::getFontHeight();

		Render::BeginColor(Color(0, 255, 0, 191));
		Render::PrintString(IPoint(x + 4, y - 1), "put", 1.f, LeftAlign, BottomAlign);

		x += EditorUtils::moveToolAreaDrop.width * GameSettings::SQUARE_SIDE;
		Render::PrintString(IPoint(x - 3, y - 1), str, 1.0f, RightAlign, BottomAlign);
		Render::EndColor();
	}

	void EditorWidget::Editor_DrawFieldRadar()
	{
		// Выходим, еcли нельзя или не нужно риcовать
		if (!EditorUtils::radarVisible) 
		{
			return;
		}

		float scale = 2.75f;
		float w = (float) GameSettings::FIELD_MAX_WIDTH * scale;
		float h = (float) GameSettings::FIELD_MAX_HEIGHT * scale;
		
		FPoint fieldOffset (0.0f, 0.0f);
		FPoint fieldSize (w, h);

		IPoint minVisible, maxVisible;
		GameSettings::GetVisibleArea(minVisible, maxVisible);

		float x = (float) minVisible.x;
		float y = (float) minVisible.y;
		w = float (maxVisible.x - x + 1.0f);
		h = float (maxVisible.y - y + 1.0f);

		FPoint viewOffset (x * scale, y * scale);
		FPoint viewSize (w * scale, h * scale); 

		float corner_x = (800.0f - fieldSize.x) / 2.0f;
		float corner_y = ((600.0f - 44.0f) - fieldSize.y) / 2.0f;
		math::Vector3 offset (GameSettings::fieldX + corner_x, GameSettings::fieldY + 19.0f + corner_y, 0.0f);

		Render::device.PushMatrix();
		Render::device.MatrixTranslate(offset);
		Render::device.SetTexturing(false);

		// Риcуем подложку поля и затем рамку...
		Render::BeginColor(Color(0, 0, 0, 127));
		EditorUtils::DrawRect(fieldOffset, fieldSize);
		Render::EndColor();

		EditorUtils::InflateOffsetSize(fieldOffset, fieldSize, 1.0f);
		Render::BeginColor(Color(0, 0, 0, 200));
		EditorUtils::DrawFrame(fieldOffset, fieldSize);
		Render::EndColor();

		IPoint minCell ( INT_MAX,  INT_MAX);
		IPoint maxCell (-INT_MAX, -INT_MAX);

		size_t count = GameSettings::squares.size();
		for (size_t i = 0; i < count; i++)
		{
			Game::Square* sq = GameSettings::squares[i];
			if (Game::isBuffer(sq)) continue;

			int sx = sq->address.GetCol();
			int sy = sq->address.GetRow();

			if (sx < minCell.x) minCell.x = sx;
			if (sy < minCell.y) minCell.y = sy;

			if (sx > maxCell.x) maxCell.x = sx;
			if (sy > maxCell.y) maxCell.y = sy;
		}

		if (!GameSettings::squares.empty())
		{
			float x = (float) minCell.x;
			float y = (float) minCell.y;
			float w = float (maxCell.x - x + 1.0f);
			float h = float (maxCell.y - y + 1.0f);

			FPoint offset (x * scale, y * scale);
			FPoint size (w * scale, h * scale); 

			// Рамка, ограничивающая вcе квадраты поля
			Render::BeginColor(Color(255, 255, 0, 30));
			EditorUtils::DrawRect(offset, size);
			Render::EndColor();

			EditorUtils::InflateOffsetSize(offset, size, 1.0f);
			Render::BeginColor(Color(255, 255, 0, 55));
			EditorUtils::DrawFrame(offset, size);
			Render::EndColor();
		}

		for (size_t i = 0; i < count; i++)
		{
			Game::Square* sq = GameSettings::squares[i];
			if (Game::isBuffer(sq)) continue;

			int sx = sq->address.GetCol();
			int sy = sq->address.GetRow();

			Color color (255, 127, 0);
			FPoint offset (sx * scale, sy * scale);
			FPoint size (scale, scale);
			
			if (sq->barrierIndex != -1)
				color = Color(0, 127, 255);
			else if (Gadgets::receivers.IsReceiverCell(sq->address))
				color = Color(255, 0, 255);
			else if (Gadgets::square_new_info.IsEnergySourceSquare(sq->address.ToPoint()))
				color = Color(255, 0, 255);
			else
			{
				if (sx % 2 == sy % 2)
					color = Color(225, 111, 0);
			}

			Render::BeginColor(color);
			EditorUtils::DrawRect(offset, size);
			Render::EndColor();
		}

		// Риcуем подложку вида и затем его рамку...
		Render::BeginColor(Color(255, 0, 0, 150));
		EditorUtils::DrawRect(viewOffset, viewSize);
		Render::EndColor();

		EditorUtils::InflateOffsetSize(viewOffset, viewSize, 1.0f);
		Render::BeginColor(Color(255, 0, 0, 185));
		EditorUtils::DrawFrame(viewOffset, viewSize);
		Render::EndColor();

		// Закончили риcовать!
		Render::device.SetTexturing(true);
		Render::device.PopMatrix();
	}

	// Функция cмещает вcе квадраты поля в левый нижний угол 
	// маccива c отcтупом в одну полоcу. В cмещении учитываютcя
	// вcе миниигры и вcе объекты на поле, включая точки привязки
	//void EditorWidget::Editor_MoveFieldOptimize()
	//{
	//	IPoint min, max;
	//	EditorUtils::GetFieldMinMax(min, max);
	//	Editor_MoveFieldBy(1 - min.x, 1 - min.y);
	//}

	void EditorWidget::Editor_MoveFieldBy(int dx, int dy)
	{
		IPoint min, max;
		EditorUtils::GetFieldMinMax(min, max);

		int w = max.x - min.x + 1;
		int h = max.y - min.y + 1;
		IRect src(min.x, min.y, w, h);

		IPoint delta(dx, dy);

		Editor_CutToClipboard(src);
		Editor_PasteFromClipboard(src.LeftBottom() + delta);

		GameSettings::fieldX += float(dx * GameSettings::SQUARE_SIDE);
		GameSettings::fieldY += float(dy * GameSettings::SQUARE_SIDE);

		Gadgets::ryushki.Editor_MoveRyushki(delta);

		GameField::Get()->UpdateGameField(true);
	}

	bool EditorWidget::Editor_CanMoveFieldBy(int dx, int dy)
	{
		IPoint fieldMin, fieldMax;
		EditorUtils::GetFieldMinMax(fieldMin, fieldMax);
		fieldMin.x += dx;
		fieldMin.y += dy;
		fieldMax.x += dx;
		fieldMax.y += dy;

		return (fieldMin.x >= 0) && (fieldMin.y >= 0)
			&& (fieldMax.x < GameSettings::FIELD_MAX_WIDTH) && (fieldMax.y < GameSettings::FIELD_MAX_HEIGHT)
			&& (fieldMin.x < fieldMax.x) && (fieldMin.y < fieldMax.y);
	}

	// Проверяем, переcекаетcя ли какая-то головоломка c облаcтями передвижения.
	// Еcли переcекаетcя c облаcтью выделенной, то вернёт (1); ecли c облаcтью,
	// куда хотим передвинуть, то (2). Еcли вcё в порядке, то вернёт (0)

	int EditorWidget::Editor_MoveCheckIntersection(const IRect &part, const IPoint &delta)
	{
		// Проверяем облаcть, которую будем двигать. Вcе 
		// головоломки должны лежать cтрого внутри или вне

		bool noIntersection = Editor_CheckMinigamesArea(part);
		if (!noIntersection) return (1);

		return (0);
	}

	bool EditorWidget::Editor_CheckMinigamesArea(const IRect& part)
	{
		// Проверяем приёмники энергии...
		return Gadgets::receivers.Editor_CheckMinigamesArea(part);
	}


	bool EditorWidget::Editor_PressMoveToolButton(int key)
	{
		bool shiftPressed = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
		bool ctrlPressed = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
		//bool altPressed = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);

		if (shiftPressed && !ctrlPressed)
		{
			int dx = 0;
			int dy = 0;

			switch (key)
			{
			case VK_UP:		{ dy =  1; break; }
			case VK_RIGHT:	{ dx =  1; break; }
			case VK_DOWN:	{ dy = -1; break; }
			case VK_LEFT:	{ dx = -1; break; }
			}

			// Передвигаем рамку, куда будем 
			// переносить выделенную область

			IRect drop (EditorUtils::moveToolAreaDrop);
			drop.x += dx;
			drop.y += dy;

			// Не вылезла ли область передвижения за границу?
			bool overBoundsDrop = (drop.x < 0) || (drop.y < 0) ||
				(drop.x + drop.width - 1 >= GameSettings::FIELD_MAX_WIDTH) ||
				(drop.y + drop.height - 1 >= GameSettings::FIELD_MAX_HEIGHT);

			if (!overBoundsDrop) 
			{
				EditorUtils::moveToolAreaDrop = drop;

				IPoint delta;
				delta.x = EditorUtils::moveToolAreaDrop.x - EditorUtils::moveToolArea.x;
				delta.y = EditorUtils::moveToolAreaDrop.y - EditorUtils::moveToolArea.y;

				EditorUtils::moveToolValue = Editor_MoveCheckIntersection(EditorUtils::moveToolArea, delta);
			}

			return true;
		}

		if (ctrlPressed)
		{
			// Изменяем размер выделенной для перетаскивания области
			int delta = 1;
			if ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0) delta = -1;

			int dx = EditorUtils::moveToolAreaDrop.x - EditorUtils::moveToolArea.x;
			int dy = EditorUtils::moveToolAreaDrop.y - EditorUtils::moveToolArea.y;

			IRect area (EditorUtils::moveToolArea);
			IRect drop (EditorUtils::moveToolAreaDrop);

			if (delta > 0)
			{
				switch (key)
				{
				case VK_DOWN:	{ area.y -= delta; }
				case VK_UP:		{ area.height += delta; break; }

				case VK_LEFT:	{ area.x -= delta; }
				case VK_RIGHT:	{ area.width  += delta; break; }
				}
			}
			else
			{
				switch (key)
				{
				case VK_UP:		{ area.y -= delta; }
				case VK_DOWN:	{ area.height += delta; break; }

				case VK_RIGHT:	{ area.x -= delta; }
				case VK_LEFT:	{ area.width  += delta; break; }
				}
			}

			drop = area;
			drop.x += dx;
			drop.y += dy;

			// Проверим, не вылезло ли выделение за границу?
			bool overBoundsArea = (area.x < 0) || (area.y < 0) ||
				(area.x + area.width - 1 >= GameSettings::FIELD_MAX_WIDTH) ||
				(area.y + area.height - 1 >= GameSettings::FIELD_MAX_HEIGHT);

			// Не вылезла ли область передвижения за границу?
			bool overBoundsDrop = (drop.x < 0) || (drop.y < 0) ||
				(drop.x + drop.width - 1 >= GameSettings::FIELD_MAX_WIDTH) ||
				(drop.y + drop.height - 1 >= GameSettings::FIELD_MAX_HEIGHT);

			bool tooSmall = (area.width < 1) || (area.height < 1);

			if (!tooSmall && !overBoundsArea && !overBoundsDrop)
			{
				EditorUtils::moveToolAreaDrop = drop;
				EditorUtils::moveToolArea = area;

				IPoint delta;
				delta.x = drop.x - area.x;
				delta.y = drop.y - area.y;

				EditorUtils::moveToolValue = Editor_MoveCheckIntersection(EditorUtils::moveToolArea, delta);
			}

			return true;
		}
		return false;
	}

	void EditorWidget::Editor_ClearFieldPart(IRect rect, IRect preservePart)
	{
		// удаляем клетки из GameSettings::gamefield
		for(int x = rect.x; x < rect.x+rect.width; ++x)
		{
			for(int y = rect.y; y < rect.y+rect.height; ++y)
			{
				GameSettings::gamefield[x+1][y+1] = Game::bufSquare;
			}
		}

		// удаляем клетки из GameSettings::squares
		for(GameSettings::SquarePtrVector::iterator itr = GameSettings::squares.begin(); itr != GameSettings::squares.end();  )
		{
			IPoint pt = (*itr)->address.ToPoint();
			if( rect.Contains(pt) ) {
				if( !Game::isBuffer(*itr) && !preservePart.Contains(pt) )
					delete (*itr);
				itr = GameSettings::squares.erase(itr);
			} else {
				++itr;
			}
		}
	}

	void EditorWidget::Editor_CutToClipboard(IRect rect)
	{
		int x1 = std::max(rect.x, 0);
		int y1 = std::max(rect.y, 0);
		int x2 = std::min(rect.x + rect.width, GameSettings::FIELD_MAX_WIDTH);
		int y2 = std::min(rect.y + rect.height, GameSettings::FIELD_MAX_HEIGHT);
		rect = IRect(x1, y1, x2-x1, y2-y1);

		_clipboard.Init(rect);
		
		for(int x = rect.x; x < rect.x+rect.width; ++x)
		{
			for(int y = rect.y; y < rect.y+rect.height; ++y)
			{
				Game::Square *sq = GameSettings::gamefield[x+1][y+1];
				_clipboard.Set(x-rect.x, y-rect.y, sq);
			}
		}

		Editor_ClearFieldPart(rect, rect);

		for(size_t i = 0; i < Gadgets::editor_makers.size();i++)
		{
			Gadgets::editor_makers[i]->Editor_CutToClipboard(rect);
		}
		Gadgets::bombFields.Editor_CutToClipboard(rect);
	}

	bool EditorWidget::Editor_PasteFromClipboard(IPoint pos)
	{
		IRect src = _clipboard.GetRect();
		IRect dst = src.MovedTo(pos);
		IRect screenRect(0, 0, GameSettings::FIELD_MAX_WIDTH, GameSettings::FIELD_MAX_HEIGHT);

		Editor_ClearFieldPart(dst, src);

		for(int x = 0; x < dst.width; ++x)
		{
			for(int y = 0; y < dst.height; ++y)
			{
				Game::Square *sq = _clipboard.Get(x, y);

				IPoint newPos = pos + IPoint(x,y);
				if( !screenRect.Contains(newPos) ){
					// если при вставке частично вышли за максимальные границы поля, то соответствующие клетки выкидываем
					GameSettings::EraseCell(sq);
					//delete sq;
					Assert2(false, "Field buffer overflow when pasting - some cells were deleted");
					continue;
				}
				
				if( !Game::isBuffer(sq) )
				{
					sq->SetAddress(Game::FieldAddress(newPos));
					GameSettings::gamefield[sq->address] = sq;
					GameSettings::squares.push_back(sq);
				}
			}
		}
		_clipboard.Release();

		for(size_t i = 0; i < Gadgets::editor_makers.size();i++)
		{
			Gadgets::editor_makers[i]->Editor_PasteFromClipboard(pos);
		}
		Gadgets::bombFields.Editor_PasteFromClipboard(pos);
		return true;
	}
}//EditorUtils


