#pragma once

#include "../RenderTargetHolder.h"
class DynamicScroller;

namespace Card
{
	class CardAvatarItem;
	class MapItem;
	class Map;
	class BigBackground;
	
	class CardWidget
		: public GUI::Widget
	{
	public:
		CardWidget(const std::string &name, rapidxml::xml_node<>* xml_elem);
		~CardWidget();
		void Update(float dt);
		void Draw();
		void MouseMove(const IPoint &mouse_pos);
		bool MouseDown(const IPoint &mouse_pos);
		void MouseUp(const IPoint &mouse_pos);
		void AcceptMessage(const Message &message);
		Message QueryState(const Message& message) const;
		void MouseWheel(int delta);
	private:
		// загрузка карты из xml (debug==true - принудительная перезагрузка)
		void Load(bool debug);
		float GetScrollWithStartOffset();
		void RenderFull();
		// освободить ресурсы карты
		void ReleaseCardResources();
		// расположить иконки друзей на карте
		void SetFriendAvatars();
	private:

		Render::ShaderProgram *_cardShader;
		RenderTargetHolder _virtualScreen, _virtualScreen2;

		bool _editor;
		bool _mouseDown;

		//Тап по объекту. Если уведем с объекта, то нужно будет скроллировать
		bool _tapScroll;
		// карта, которая содержит в себе все объекты
		Map* map;

		typedef std::vector<MapItem*> MapItemList;
		// объекты, которые рисуются в данный момент
		MapItemList draw_objects;
		// объекты, которые обновляются в данный момент, их чуть-чуть больше (чтобы объеты не застывали после смещения экрана)
		MapItemList update_objects;

		// прямоугольник для объектов, попадающих под рисование
		IRect draw_rect;
		// смещение draw_rect относительно offset_screen
		int draw_rect_shift;
		// прямоугольник для объектов, попадающих под обновление
		IRect update_rect;
		// смещение update_rect относительно offset_screen
		int update_rect_shift;
		// заполняет / удаляет объеты, попадающие / непопадающие в требуемые области
		void updateItems(bool obviously);

		// нужно ли запускать анимации открытия уровней?
		bool open_level_visualization;
		
		float _scrollPos;

		IPoint mouseDownPosition;
		// задник
		BigBackground* background;
		// двигает карту
		DynamicScroller* scroll;
	};
}