#ifndef GAME_LIGHTNING_CONTROLLER_H
#define GAME_LIGHTNING_CONTROLLER_H

#include "GameFieldControllers.h"

namespace Game
{
	class GameLightningController
		: public GameFieldController
	{
		IPoint _startSquaerAddress;
		int _color;
		Color _colorize;
		Render::Texture *_stripTex;
		Render::Texture *_backStripTex;
		StripEffect _strip;
		StripEffect _backStrip;
		FPoint _center_explision;
		float _localTime;
		float _procesTime;
		std::vector<Game::Square*> _csquares;
	public:
		GameLightningController(const IPoint &start, int color, float startTime);
		~GameLightningController();
		void Update(float dt);
		void Draw();
		bool isFinish();
		static bool ValidSquare(Game::Square *sq, const int &color);
		std::vector<Game::Square*>* GetSquares();
		float GetTimeScale() const;
	private:
		void DoLightning();
	};

	class GameSplashLightningController : public GameFieldController
	{
	public:
		typedef std::vector<FPoint> Points;

	private:
		float STATIC_TIME;				// время в течении которого _offSet постоянно
		float _offSetTimeCounter;		// счетчик этого времени
		Render::Texture *_splash;		// текстура молнии
		Render::Texture *_splashDot;	// текстура концов молнии
		Points _way;					// точки через которые проходит молния
		float _offSet;					// смещение текстурных координат
		float _time;					// время жизни
		float _timeCounter;				// счетчик времени от _time до 0
		float _start;					// начало молнии
		float _length;					// длина всей траектории молнии
		long _soundId;					// идентификатор зацикленного звука молнии
		float _delay;
		std::vector<FPoint> _startTimes;
		AddressVector _cells;
		std::vector<bool> shiftingCells;

		struct PolarPoint
		{
			float alpha;
			float distance;
			int index;
			Game::FieldAddress fa;
		};

		void Swap(IPoint &a, IPoint &b);
		static bool CmpPolar(PolarPoint a, PolarPoint b);
		static bool CmpPolarRevers(PolarPoint a, PolarPoint b);

	public:
		GameSplashLightningController(float time, AddressVector &points, float delay = 0);
		~GameSplashLightningController();

		void Update(float dt);
		void DrawAbsolute();
		bool isFinish();
	};

} //namespace Game
#endif //GAME_LIGHTNING_CONTROLLER_H