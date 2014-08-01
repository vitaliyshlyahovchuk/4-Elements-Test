#ifndef ONCE_FLY_TEXT_BY_SPLINE_
#define ONCE_FLY_TEXT_BY_SPLINE_

#include "GameFieldController.h"

namespace Match3GUI
{
	class FlyTextBySpline
		:public GameFieldController
	{
		std::string _text;
		SplinePath<FPoint> _spline;
		float _timer, _time_scale;
		bool _absolute;
		FPoint _pos;
		Render::Texture *_backTexture;
		FPoint _backCenter;

		Message _messageEnd;
		GUI::Widget *_messageWidget;

		TextAlign _va, _ha;
	public:
		FlyTextBySpline(const std::string  &text, const FPoint &pos_from, const FPoint &pos_to, float time_scale, GameField *gamefield_, const bool &absolute, Render::Texture *back_texture = NULL);
		void Draw();
		void DrawAbsolute();
		void Update(float dt);
		bool isFinish();
		void AddMessage(const Message &message, GUI::Widget *widget);
	};
} //Match3GUI

#endif //ONCE_FLY_TEXT_BY_SPLINE_