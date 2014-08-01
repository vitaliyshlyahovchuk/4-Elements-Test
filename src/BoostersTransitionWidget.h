#pragma once

class BoostersTransitionWidget
	: public GUI::Widget
{
private:
	Render::Texture *_boosterBackTex;			// �������� �������� ��� ������� �������
	int _boosters_count;						// ���������� ������������ ��������
	float _timer;								// ������ �����
	int _boostsReadyToNextState;				// ���������� ������ ������� � ���������� ���������
	bool _boostersLoaded;						// ��������� �� ���������� � ������
	EffectsContainer _effCont;					// �������

	enum State {
		NONE, MOVEIN, WAIT,
		RUNING, MOVEOUT, FINISHED
	} _state;									// ���������

	struct FlyingBooster						// ������� ������
	{
		std::string name;						// ��� ����� (������������ ��� �������)
		std::string textureid;					// id �������� ������
		Render::Texture *boosterIconTex;		// �������� ������
		IPoint iconOffset;						// �������� ������ ������������ ��������
		FPoint pos;								// ������� �������
		float timeIn;							// ����� ���������
		float timeOut;							// ����� ������������
		SplinePath<FPoint> pathIn;				// ������ �����-���������
		SplinePath<FPoint> pathOut;				// ������ �����-������������
		State state;							// ���������
		float usageTime;						// ����� �������������
		float waitTime;							// ����� �������� ������ �������������
		std::string runEffectName;				// ��� ������� ��� ������� �����
		int pathoutType;						// ���� ������� ����� ������ ��������
		int waitY;								// Y ����������� � ������� ����� ������ �� ������ ��������
	};
	std::vector<FlyingBooster> _allflyingBoosters;

	std::vector<FlyingBooster> _flyingBoosters; //������� �������

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

