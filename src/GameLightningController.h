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
		float STATIC_TIME;				// ����� � ������� �������� _offSet ���������
		float _offSetTimeCounter;		// ������� ����� �������
		Render::Texture *_splash;		// �������� ������
		Render::Texture *_splashDot;	// �������� ������ ������
		Points _way;					// ����� ����� ������� �������� ������
		float _offSet;					// �������� ���������� ���������
		float _time;					// ����� �����
		float _timeCounter;				// ������� ������� �� _time �� 0
		float _start;					// ������ ������
		float _length;					// ����� ���� ���������� ������
		long _soundId;					// ������������� ������������ ����� ������
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