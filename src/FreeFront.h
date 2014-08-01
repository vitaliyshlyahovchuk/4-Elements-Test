#ifndef _FREE_FRONT_H_
#define _FREE_FRONT_H_
#include "Game.h"


struct ExhaustiveRoundWayElement
{
	//Game::FieldAddress a;
	Game::FieldAddress a_checking;
	bool border;
	size_t debug_dir; //�����������, ����� ������ ��� �������� ��������� �� ����
	ExhaustiveRoundWayElement()
		: border(false)
	{
	
	}
	void Init(const Game::FieldAddress &new_a, const size_t &dir, const bool &use_current_addres);
	Game::FieldAddress GetChildAddres();
	void DrawEdit();
};

class FreeFrontDetector
{
public:
	//�����. �������� �������� - ����� ����� ����� ��������.
	std::vector<ExhaustiveRoundWayElement> _freeFrontForEnergy;
	bool _cameraIsStand;
public:
	void DrawEdit();
	void LoadLevel();

	//�������������� ��� ������ ���� ����� ������� �������
	void Update();

	//use_current_addres - ��� �������� ����� ������������ ��� �� ����� �����
	void GrowFront(ExhaustiveRoundWayElement &element, const bool &use_current_addres);
	bool CameraIsStand();
	void SetCameraIsStand();
};


namespace Gadgets
{
	// ������� ��cc������ ������ �� �������� ��� c������ ��������
	extern Array2D<float> squareDistRec;  //����������� ���������� �� ��������!
	extern Array2D<float> squareDist;	  //(�������� � ���������, ����� �� ��������� �� ��� �������

	void FreeFrontInit();
	void InitProcessSettings();

	//����� ������������ ����� ������� ��� �������� ������� ������ ��������� ���� �� ������ ��� ������ � ����� ���� ������� ������� � ���� ������� �� ����� �����
	extern FreeFrontDetector freeFrontDetector;

} //Gadgets

#endif //_FREE_FRONT_H_