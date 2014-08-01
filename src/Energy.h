#pragma once

#include "Array2D.h"
#include "GameFieldAddress.h"

struct SnapGadgetReleaseBorderElement;


namespace Energy
{

struct Settings
{
	static Color color;
	static Color frontColor;
	static float part_k1, part_k2;
	static float delayTime;
	static float delaySpeed;
	static float particleDensity;
	static float particleLifetime;
	static float energyThreshold;
	static float frontThreshold;

	static void Load(rapidxml::xml_node<> *settings);
};

struct Part
{
	float e;
	float k;

	Part();
};

class Square
{
public:
	enum Segment {
		CENTER         = 0,
		LEFT_BOTTOM    = 1,
		LEFT_TOP       = 2,
		RIGHT_BOTTOM   = 3,
		RIGHT_TOP      = 4,
		LB_INNER       = 5,
		LT_INNER       = 6,
		RB_INNER       = 7,
		RT_INNER       = 8,
		TOTAL_SEGMENTS = 9
	};
private:
	Part *_parts;
	int _parts_num[TOTAL_SEGMENTS];

	bool _is_source;

	bool _energy_flowing;

	float _delayTimer;

	Game::FieldAddress _address;

	BYTE _walls;

	std::map<SnapGadgetReleaseBorderElement*, bool> _snaps_listeners_on_energy_fill;

	Part& getPart(int i, int j) const;

	void InitEnergy();
	void FreeEnergy();

	void OnSquareFilled();

	void DrawQuad(int quad);

	int TotalParts() const;

public:
	Square(Game::FieldAddress addr);
	~Square();

	void Draw();
	void AddEnergy(int i, int j);

	float getEnergy(int i, int j) const;

	Game::FieldAddress getAddress() const;

	// проверяет заполненность энергией в одном из сегментов клетки
	bool FullOfEnergy(int quad = CENTER) const;
	bool EnergyExists(int quad = CENTER, const int value = 1) const;

	void StartEnergySource();

	// пересчитывает, в какие зоны клетки может затекать энергия, инициализирует или наоборот освобождает частицы
	bool UpdateWalls();

	void UpdateEnergyWalls();

	// отвечает за течение энергии
	bool Update(float dt);

	//Добавляем сигнал на событие полного заполнения энергией определеной клетки
	bool AddSnapListenerOnEnergyFill(SnapGadgetReleaseBorderElement *snap_element);
};

class Field
{
	Array2D<Square*> _squares;
	std::vector<Square*> _squaresv;

	Square *bufEnergySquare;

	bool _energy_flowing;
	bool _update_buffers;

	long _energyMoveTrackId; //код трека движения энергии

	SplinePath<float> _frontParticleScale;
	SplinePath<float> _frontParticleAlpha;

public:
	// установить признак течет ли энергия. напрямую поле не присваивать, чтобы не испортить музыку!!!
	void SetEnergyFlowing(bool value);
	static FPoint cp[16];
public:
	Field();
	~Field();

	void Init();
	void Release();

	float getEnergy(int i, int j) const;

	// создает источник энергии в квадрате
	void StartEnergyInSquare(Game::FieldAddress addres);

	// проверяет может ли быть энергия в квадрате, создает частицы или
	// наоборот их очищает, если клетка заросла или освободилась от земли
	void UpdateSquare(Game::FieldAddress address);

	// заполнен ли энергией центр клетки
	bool FullOfEnergy(Game::FieldAddress address, int quad = Square::CENTER) const;

	// есть ли энергия в центре клетки
	bool EnergyExists(Game::FieldAddress address, int quad = Square::CENTER) const;

	// течет ли энергия по полю в данный момент
	bool EnergyIsFlowing() const;

	void Update(float dt);

	void Draw();
	void DrawFront();
	void DrawParticles();

	// может ли энергия течь из одной клетки в другую (проверяет только соседние клетки)
	bool CanFlowFromTo(Game::FieldAddress from, Game::FieldAddress to) const;

	// рассчитываем какое максимальное расстояние энергии потребуется протечь, начиная с этого момента
	float CalcDistanceToFlow() const;

	//Добавляем сигнал на событие полного заполнения энергией определеной клетки
	bool AddSnapListenerOnEnergyFill(Game::FieldAddress address, SnapGadgetReleaseBorderElement *snap_element);
};

extern Field field;

} // namespace Energy