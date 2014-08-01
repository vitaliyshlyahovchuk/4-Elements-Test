#pragma once

class BoostersTransitionWidget
	: public GUI::Widget
{
private:
	Render::Texture *_boosterBackTex;			// текстура подложки под иконкой бустера
	int _boosters_count;						// количество используемых бустеров
	float _timer;								// таймер полёта
	int _boostsReadyToNextState;				// количество бустов готовых к следующему состоянию
	bool _boostersLoaded;						// загружена ли информация о бустах
	EffectsContainer _effCont;					// эффекты

	enum State {
		NONE, MOVEIN, WAIT,
		RUNING, MOVEOUT, FINISHED
	} _state;									// состояние

	struct FlyingBooster						// летящий бустер
	{
		std::string name;						// имя буста (используется для запуска)
		std::string textureid;					// id текстуры иконки
		Render::Texture *boosterIconTex;		// текстура иконки
		IPoint iconOffset;						// смещение иконки относительно подложки
		FPoint pos;								// текущая позиция
		float timeIn;							// время появления
		float timeOut;							// время исчезновения
		SplinePath<FPoint> pathIn;				// сплайн полёта-появления
		SplinePath<FPoint> pathOut;				// сплайн полёта-исчезновения
		State state;							// состояние
		float usageTime;						// время использования
		float waitTime;							// время ожидания старта использования
		std::string runEffectName;				// имя эффекта при запуске буста
		int pathoutType;						// куда полетит после экрана загрузки
		int waitY;								// Y коорддината в которой будут иконка на экране загрузки
	};
	std::vector<FlyingBooster> _allflyingBoosters;

	std::vector<FlyingBooster> _flyingBoosters; //летящие бустеры

public:
	BoostersTransitionWidget(const std::string& name_, rapidxml::xml_node<>* xmlElement);

	void Init(const Message& message);
	void LoadBoosters();
	void Update(float dt);
	void Draw();
	bool MouseDown(const IPoint& mouse_pos);
	void MouseUp(const IPoint &mouse_pos);
	void AcceptMessage(const Message& message);
};

