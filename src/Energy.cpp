#include "stdafx.h"
#include "Energy.h"
#include "GameField.h"
#include "Game.h"
#include "GameOrder.h"
#include "Tutorial.h"
#include "FieldBears.h"
#include "CellWalls.h"
#include "EnergyReceivers.h"
#include "Match3Gadgets.h"
#include "RoomGates.h"
#include "SquareNewInfo.h"

namespace Energy
{

namespace Front
{

	struct ParticlePreset
	{
		Render::Texture *tex;
		Render::BlendMode blend;
		float density;
		float lifetime[2];
		float angle[2];
		float size[2];
		float alpha[2];
		Color color;

		ParticlePreset(rapidxml::xml_node<> *node)
		{
			std::string texID = Xml::GetStringAttribute(node, "texture");
			tex = Core::resourceManager.Get<Render::Texture>(texID);
			density = Xml::GetFloatAttributeOrDef(node, "density", 0.0f);

			std::string blendStr = Xml::GetStringAttributeOrDef(node, "blend", "alpha");
			blend = (blendStr == "add") ? Render::ADD : Render::ALPHA;

			lifetime[0] = Xml::GetFloatAttributeOrDef(node, "minLifetime", 1.0f);
			lifetime[1] = Xml::GetFloatAttributeOrDef(node, "maxLifetime", 2.0f);
			Assert(lifetime[1] >= lifetime[0]);

			angle[0] = Xml::GetFloatAttributeOrDef(node, "minAngle", 0.0f);
			angle[1] = Xml::GetFloatAttributeOrDef(node, "maxAngle", 0.0f);
			Assert(angle[1] >= angle[0]);

			size[0] = Xml::GetFloatAttributeOrDef(node, "minSize", 0.5f);
			size[1] = Xml::GetFloatAttributeOrDef(node, "maxSize", 1.5f);
			Assert(size[1] >= size[0]);

			alpha[0] = Xml::GetFloatAttributeOrDef(node, "minAlpha", 0.0f);
			alpha[1] = Xml::GetFloatAttributeOrDef(node, "maxAlpha", 1.0f);
			Assert(alpha[1] >= alpha[0]);

			std::string colorStr = Xml::GetStringAttributeOrDef(node, "color", "#FFFFFF");
			color = Color(colorStr);
		}
	};

	std::vector<ParticlePreset> particleTypes;

	struct Particle
	{
		FPoint pos;
		float local_time;

		int type;

		float lifetime;
		float size;
		float alpha;
		float angle;

		Particle(FPoint pos_, int type_, const ParticlePreset &p) : pos(pos_), type(type_), local_time(0.0f)
		{
			lifetime = math::random(p.lifetime[0], p.lifetime[1]);
			size = math::random(p.size[0], p.size[1]);
			alpha = math::random(p.alpha[0], p.alpha[1]);
			angle = math::random(p.angle[0], p.angle[1]);
		}
	};

	std::vector< std::list<Particle> > particles;

} // namespace Front

const int ENERGY_PARTS_SQUARE_SIDE = 9;

Color Settings::color = Color(10, 110, 210, 255);
Color Settings::frontColor = Color(210, 210, 255, 255);
float Settings::part_k1 = 5.0f;
float Settings::part_k2 = 10.0f;
float Settings::delayTime = 0.0f;
float Settings::delaySpeed = 1.0f;
float Settings::particleDensity = 0.5f;
float Settings::particleLifetime = 1.0f;
float Settings::energyThreshold = 0.1f;
float Settings::frontThreshold = 0.05f;

void Settings::Load(rapidxml::xml_node<> *settings)
{
	rapidxml::xml_node<> *elem = settings->first_node("Color");
	if( elem ) {
		color = Color(elem);
	}

	elem = settings->first_node("Speed");
	if( elem ) {
		part_k1 = Xml::GetFloatAttributeOrDef(elem, "k1", 5.0f);
		part_k2 = Xml::GetFloatAttributeOrDef(elem, "k2", 10.0f);
	}

	elem = settings->first_node("Delay");
	if( elem ) {
		delayTime = Xml::GetFloatAttributeOrDef(elem, "time", 0.0f);
		delaySpeed = Xml::GetFloatAttributeOrDef(elem, "speed", 1.0f);
	}


	rapidxml::xml_node<> *frontElem = settings->first_node("Front");
	if( frontElem )
	{
		elem = frontElem->first_node("Color");
		if( elem ) {
			frontColor = Color(elem);
		}

		elem = frontElem->first_node("Particle");
		while( elem ) {
			Front::particleTypes.push_back( Front::ParticlePreset(elem) );
			elem = elem->next_sibling("Particle");
		}

		float width = Xml::GetFloatAttributeOrDef(frontElem, "width", 1.0f);
		energyThreshold = 0.1f;
		frontThreshold = math::lerp(0.001f, energyThreshold, math::clamp(0.0f, 1.0f, 1.0f - width));
	}
}

//// предрассчитанные точки на окружности
FPoint Field::cp[16];

Field field;

static int max_parts_quad[Square::TOTAL_SEGMENTS];
static int max_parts_total;

static const int part_quads[ENERGY_PARTS_SQUARE_SIDE][ENERGY_PARTS_SQUARE_SIDE] =
{
	{1, 5, 5, 0, 0, 0, 7, 7, 3},
	{5, 5, 0, 0, 0, 0, 0, 7, 7},
	{5, 0, 0, 0, 0, 0, 0, 0, 7},
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0},
	{6, 0, 0, 0, 0, 0, 0, 0, 8},
	{6, 6, 0, 0, 0, 0, 0, 8, 8},
	{2, 6, 6, 0, 0, 0, 8, 8, 4},
};

struct Quad
{
	FPoint p[4];
	Quad(FPoint p0, FPoint p1, FPoint p2, FPoint p3)
	{
		p[0] = p0;
		p[1] = p1;
		p[2] = p2;
		p[3] = p3;
	}
};

typedef std::vector<Render::QuadVert> VertexData;
VertexData vert_buffer;
VertexData vb_front;

void AddTriangleToVB(VertexData &vb, FPoint p0, FPoint p1, FPoint p2, Color color = Color::WHITE, float z_offset = 0.0f)
{
	vb.push_back( Render::QuadVert(p0.x, p0.y - 2.0f, ZBuf::ENERGY + z_offset, color, 0.0f, 0.0f) );
	vb.push_back( Render::QuadVert(p1.x, p1.y - 2.0f, ZBuf::ENERGY + z_offset, color, 0.0f, 0.0f) );
	vb.push_back( Render::QuadVert(p2.x, p2.y - 2.0f, ZBuf::ENERGY + z_offset, color, 0.0f, 0.0f) );
}

void AddQuadToVB(VertexData &vb, FPoint p0, FPoint p1, FPoint p2, FPoint p3, Color color = Color::WHITE, float z_offset = 0.0f)
{
	AddTriangleToVB(vb, p0, p1, p2, color, z_offset);
	AddTriangleToVB(vb, p2, p1, p3, color, z_offset);
}


Part::Part()
	: e(0.0f)
{
	k = math::random(Settings::part_k1, Settings::part_k2);
}

// в каком квадранте находится частица (центральный круг или один из 4-х уголков)
inline int PartQuadrant(int i, int j)
{
	return part_quads[j][i];
}

static void DrawMarchingSquare(VertexData &vb, FRect rect, float e1, float e2, float e3, float e4, float threshold, Color color = Color::WHITE, float z_offset = 0.0f)
{
	/* e1 - e2
	   |    |
	   e3 - e4 */
	BYTE type = 0;
	if( e1 >= threshold ) type |= 1;
	if( e2 >= threshold ) type |= 2;
	if( e3 >= threshold ) type |= 4;
	if( e4 >= threshold ) type |= 8;


	FPoint p0 = rect.LeftBottom();
	FPoint p1 = rect.LeftTop();
	FPoint p2 = rect.RightBottom();
	FPoint p3 = rect.RightTop();

	FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
	FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
	FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
	FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));

	/* p1 - pt4 - p3
	   |          |
	   pt1        pt3
	   |          |
	   p0 - pt2 - p2 */

	if( type == 0 )
	{
		// ..
		// ..
		// nothing to draw
	}
	else if( type == 4 )
	{
		// ..
		// *.
		FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
		FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
		AddTriangleToVB(vb, p0, pt1, pt2, color, z_offset);
	}
	else if( type == 8 )
	{
		// ..
		// .*
		FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
		FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
		AddTriangleToVB(vb, pt2, p2, pt3, color, z_offset);
	}
	else if( type == 12 )
	{
		// ..
		// **
		FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
		FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
		AddQuadToVB(vb, p0, pt1, p2, pt3, color, z_offset);
	}
	else if( type == 2 )
	{
		// .*
		// ..
		FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
		FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));
		AddTriangleToVB(vb, pt4, pt3, p3, color, z_offset);
	}
	else if( type == 6 )
	{
		// .*
		// *.
		FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
		FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
		FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
		FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));
		AddQuadToVB(vb, p0, pt1, pt2, pt4, color, z_offset);
		AddQuadToVB(vb, pt2, pt4, pt3, p3, color, z_offset);
	}
	else if( type == 10 )
	{
		// .*
		// .*
		FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
		FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));
		AddQuadToVB(vb, pt2, pt4, p2, p3, color, z_offset);
	}
	else if( type == 14 )
	{
		// .*
		// **
		FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
		FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));
		AddTriangleToVB(vb, p0, pt1, pt4, color, z_offset);
		AddQuadToVB(vb, p0, pt4, p2, p3, color, z_offset);
	}
	else if( type == 1 )
	{
		// *.
		// ..
		FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
		FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));
		AddTriangleToVB(vb, pt1, p1, pt4, color, z_offset);
	}
	else if( type == 5 )
	{
		// *.
		// *.
		FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
		FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));
		AddQuadToVB(vb, p0, p1, pt2, pt4, color, z_offset);
	}
	else if( type == 9 )
	{
		// *.
		// .*
		FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
		FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
		FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
		FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));
		AddQuadToVB(vb, pt1, p1, pt2, pt4, color, z_offset);
		AddQuadToVB(vb, pt2, pt4, p2, pt3, color, z_offset);
	}
	else if( type == 13 )
	{
		// *.
		// **
		FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
		FPoint pt4 = (e2 > e1) ? math::lerp(p1, p3, (threshold - e1) / (e2 - e1)) : math::lerp(p3, p1, (threshold - e2) / (e1 - e2));
		AddQuadToVB(vb, p0, p1, p2, pt4, color, z_offset);
		AddTriangleToVB(vb, p2, pt4, pt3, color, z_offset);
	}
	else if( type == 3 )
	{
		// **
		// ..
		FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
		FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
		AddQuadToVB(vb, pt1, p1, pt3, p3, color, z_offset);
	}
	else if( type == 7 )
	{
		// **
		// *.
		FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
		FPoint pt3 = (e4 > e2) ? math::lerp(p3, p2, (threshold - e2) / (e4 - e2)) : math::lerp(p2, p3, (threshold - e4) / (e2 - e4));
		AddQuadToVB(vb, p0, p1, pt2, p3, color, z_offset);
		AddTriangleToVB(vb, pt2, p3, pt3, color, z_offset);
	}
	else if( type == 11 )
	{
		// **
		// .*
		FPoint pt1 = (e3 > e1) ? math::lerp(p1, p0, (threshold - e1) / (e3 - e1)) : math::lerp(p0, p1, (threshold - e3) / (e1 - e3));
		FPoint pt2 = (e3 > e4) ? math::lerp(p2, p0, (threshold - e4) / (e3 - e4)) : math::lerp(p0, p2, (threshold - e3) / (e4 - e3));
		AddTriangleToVB(vb, pt1, p1, p3, color, z_offset);
		AddQuadToVB(vb, pt2, pt1, p2, p3, color, z_offset);
	}
	else if( type == 15 )
	{
		// **
		// **
		AddQuadToVB(vb, p0, p1, p2, p3, color, z_offset);
	}
	else
	{
		Assert(false);
	}
}

Square::Square(Game::FieldAddress addr)
	: _parts(nullptr)
	, _address(addr)
	, _walls(0)
	, _is_source(false)
	, _energy_flowing(false)
	, _delayTimer(0.0f)
{
	for(int i = 0; i < TOTAL_SEGMENTS; ++i) {
		_parts_num[i] = -1;
	}

	UpdateEnergyWalls();

	_snaps_listeners_on_energy_fill.clear();
}

Square::~Square()
{
	if(_parts)
		FreeEnergy();
	_snaps_listeners_on_energy_fill.clear();
}

void Square::InitEnergy()
{
	Assert(_parts == nullptr);
	_parts = new Part[ENERGY_PARTS_SQUARE_SIDE * ENERGY_PARTS_SQUARE_SIDE];
}

void Square::FreeEnergy()
{
	delete[] _parts;
	_parts = nullptr;
	_energy_flowing = false;
}

Part& Square::getPart(int i, int j) const
{
	Assert(i >= 0 && i < ENERGY_PARTS_SQUARE_SIDE);
	Assert(i >= 0 && j < ENERGY_PARTS_SQUARE_SIDE);
	Assert(_parts);
	return _parts[i * ENERGY_PARTS_SQUARE_SIDE + j];
}

float Square::getEnergy(int i, int j) const
{
	if(_parts)
		return getPart(i, j).e;
	else
		return (_parts_num[0] > 0) ? 1.0f : 0.0f;
}

Game::FieldAddress Square::getAddress() const
{
	return _address;
}


void Square::StartEnergySource()
{
	const int i = ENERGY_PARTS_SQUARE_SIDE / 2;
	if(_parts)
	{
		Part &p = getPart(i, i);
		if(p.e < 1.0f)
		{
			p.e = 1.0f;
			int q = PartQuadrant(i, i);
			_parts_num[q]++;
		}
	}
	else
	{
		Halt("Trying to start energy on invalid square, check energy sources positions on level");
	}
	_is_source = true;
}

int SquareEnergyType(Game::Square *sq)
{
	if( sq->CanEnergy() ) {
		return 0;
	} else {
		return 1;
	}
}

bool CanEnergyInCorner(int corner, Game::Square *sq1, Game::Square *sq2, Game::Square *sq3, Game::Square *sq4)
{
	const BYTE corners[] =
	{
		0xF, 0xF, 0xF, 0xF, 0xC, 0xF, 0xF, 0xF, 0xC,
		0xF, 0xA, 0xF, 0xF, 0x0, 0xF, 0xF, 0xA, 0xC,
		0xF, 0xF, 0xA, 0xF, 0xC, 0xA, 0xF, 0xF, 0x0,
		0xF, 0xF, 0xF, 0x5, 0x0, 0x5, 0xF, 0xF, 0xC,
		0x3, 0x0, 0x3, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0,
		0xF, 0xF, 0xA, 0x5, 0x0, 0x0, 0xF, 0xF, 0x0,
		0xF, 0xF, 0xF, 0xF, 0xC, 0xF, 0x5, 0x5, 0x0,
		0xF, 0xA, 0xF, 0xF, 0x0, 0xF, 0x5, 0x0, 0x0,
		0x3, 0x3, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0
	};

	int idx = SquareEnergyType(sq1) * 27 +
	          SquareEnergyType(sq2) * 9 +
	          SquareEnergyType(sq3) * 3 +
	          SquareEnergyType(sq4);

	if( corner == Square::LEFT_TOP )
		return (corners[idx] & 1);
	if( corner == Square::RIGHT_TOP )
		return (corners[idx] & 2);
	if( corner == Square::LEFT_BOTTOM )
		return (corners[idx] & 4);
	if( corner == Square::RIGHT_BOTTOM )
		return (corners[idx] & 8);

	return false;
}

bool Square::UpdateWalls()
{
	if( !_address.IsValid() )
		return false;


	Game::Square *sq = GameSettings::gamefield[_address];

	Game::Square *up = GameSettings::gamefield[_address.Up()];
	Game::Square *down = GameSettings::gamefield[_address.Down()];
	Game::Square *left = GameSettings::gamefield[_address.Left()];
	Game::Square *right = GameSettings::gamefield[_address.Right()];
	Game::Square *ld = GameSettings::gamefield[_address.Shift(-1,-1)];
	Game::Square *lu = GameSettings::gamefield[_address.Shift(-1,1)];
	Game::Square *rd = GameSettings::gamefield[_address.Shift(1,-1)];
	Game::Square *ru = GameSettings::gamefield[_address.Shift(1,1)];

	bool canEnergy[TOTAL_SEGMENTS];
	bool changed = false;

	canEnergy[CENTER] = false;
	if( sq->CanEnergy())
	{
		EnergyReceiver *rcv = Gadgets::receivers.GetReceiverOnSquare(_address);
		if( !rcv || rcv->IsActivatedByEnergy() || !rcv->IsOrdered())
			canEnergy[CENTER] = true;
	}
	canEnergy[RIGHT_TOP] = CanEnergyInCorner(RIGHT_TOP, up, ru, sq, right);
	canEnergy[LEFT_TOP] = CanEnergyInCorner(LEFT_TOP, lu, up, left, sq);
	canEnergy[RIGHT_BOTTOM] = CanEnergyInCorner(RIGHT_BOTTOM, sq, right, down, rd);
	canEnergy[LEFT_BOTTOM] = CanEnergyInCorner(LEFT_BOTTOM, left, sq, ld, down);

	canEnergy[LT_INNER] = canEnergy[CENTER] || canEnergy[LEFT_TOP];
	canEnergy[LB_INNER] = canEnergy[CENTER] || canEnergy[LEFT_BOTTOM];
	canEnergy[RT_INNER] = canEnergy[CENTER] || canEnergy[RIGHT_TOP];
	canEnergy[RB_INNER] = canEnergy[CENTER] || canEnergy[RIGHT_BOTTOM];

	for(int i = 0; i < TOTAL_SEGMENTS; ++i)
	{
		int parts = canEnergy[i] ? std::max(0, _parts_num[i]) : -1;
		changed |= (_parts_num[i] != parts);
		_parts_num[i] = parts;
	}

	int total_parts = TotalParts();
	if(total_parts >= max_parts_total || total_parts == -TOTAL_SEGMENTS)
	{
		if(_parts)
			FreeEnergy();
	}
	else
	{
		if( !_parts)
			InitEnergy();

		for(int i = 0; i < ENERGY_PARTS_SQUARE_SIDE; ++i)
		{
			for(int j = 0; j < ENERGY_PARTS_SQUARE_SIDE; ++j)
			{
				int q = PartQuadrant(i,j);
				if( !canEnergy[q] ) {
					getPart(i,j).e = 0.0f;
				}
			}
		}
	}

	if( changed ) {
		_delayTimer = Settings::delayTime;
	}

	UpdateEnergyWalls();

	return changed;
}

void Square::UpdateEnergyWalls()
{
	if(_address.IsValid() )
		_walls = Gadgets::cellWalls.EnergyWallsInCell(_address) | Gadgets::gates.EnergyWallsInCell(_address);
}

static float de[10][10];

bool Square::Update(float dt)
{
	_energy_flowing = false;

	if( _delayTimer > 0.0f ) {
		_delayTimer -= dt;
		dt *= Settings::delaySpeed;
	}

	if( !_parts )
		return false;

	IPoint offset = _address.ToPoint() * ENERGY_PARTS_SQUARE_SIDE;

	bool was_full = FullOfEnergy();

	for(int i = 0; i < ENERGY_PARTS_SQUARE_SIDE; ++i)
	{
		for(int j = 0; j < ENERGY_PARTS_SQUARE_SIDE; ++j)
		{
			de[i][j] = 0.0f;

			Part &part = getPart(i,j);

			if(_parts_num[PartQuadrant(i,j)] < 0)
				continue;

			if( part.e < 1.0f )
			{
				float e1 = ((i > 0) || ((_walls & Gadgets::CellWalls::LEFT) == 0)) ? field.getEnergy(offset.x + i - 1, offset.y + j) : 0.0f;
				float e2 = ((j < ENERGY_PARTS_SQUARE_SIDE-1) || ((_walls & Gadgets::CellWalls::UP) == 0)) ? field.getEnergy(offset.x + i, offset.y + j + 1) : 0.0f;
				float e3 = ((i < ENERGY_PARTS_SQUARE_SIDE-1) || ((_walls & Gadgets::CellWalls::RIGHT) == 0)) ? field.getEnergy(offset.x + i + 1, offset.y + j) : 0.0f;
				float e4 = ((j > 0) || ((_walls & Gadgets::CellWalls::DOWN) == 0)) ? field.getEnergy(offset.x + i, offset.y + j - 1) : 0.0f;

				if( e1 < part.e ) e1 = 0.0f;
				if( e2 < part.e ) e2 = 0.0f;
				if( e3 < part.e ) e3 = 0.0f;
				if( e4 < part.e ) e4 = 0.0f;

				de[i][j] = (e1 + e2 + e3 + e4) * part.k * dt;
			}
		}
	}

	for(int i = 0; i < ENERGY_PARTS_SQUARE_SIDE; ++i)
	{
		for(int j = 0; j < ENERGY_PARTS_SQUARE_SIDE; ++j)
		{
			Part &part = getPart(i,j);
			if( de[i][j] > 0.0f && part.e < 1.0f )
			{
				_energy_flowing = true;
				part.e = std::min(part.e + de[i][j], 1.0f);

				if(part.e >= 1.0f )
				{
					int q = PartQuadrant(i,j);
					_parts_num[q]++;
				}

				// генерируем частицы во фронте энергии
				if(part.e >= Settings::energyThreshold && part.e - de[i][j] < Settings::energyThreshold)
				{
					for(size_t t = 0; t < Front::particleTypes.size(); ++t)
					{
						const Front::ParticlePreset &p = Front::particleTypes[t];
						if( math::random(0.0f, 1.0f) < p.density )
						{
							float k = 1.0f / ENERGY_PARTS_SQUARE_SIDE;
							IPoint offset = _address.ToPoint() * ENERGY_PARTS_SQUARE_SIDE;
							FPoint pos = FPoint(offset.x + i, offset.y + j) * GameSettings::SQUARE_SIDEF * k + GameSettings::CELL_HALF * k;
							FPoint rnd_offset = FPoint(math::random(-1.0f, 1.0f), math::random(-1.0f, 1.0f)) * k * GameSettings::SQUARE_SIDEF * 0.5f;
							Front::particles[t].push_back( Front::Particle(pos + rnd_offset, t, p) );
						}
					}
				}
			}
		}
	}

	// клетка только что заполнилась энергией
	if( !was_full && FullOfEnergy() ) {
		OnSquareFilled();
	}

	// помечаем, что в этой клетке была энергия (потом она может там и высохнуть), для камеры
	if( EnergyExists() ){
		GameSettings::gamefield[_address]->SetEnergyChecked(false, 3.2f);
	}

	if( TotalParts() >= max_parts_total )
	{
		FreeEnergy();
	}

	return _energy_flowing;
}

// обработчик события, что клетка (ее центр) полностью заполнилась энергией
void Square::OnSquareFilled()
{
	Game::Orders::KillCell(Game::Order::ENERGY, _address);

	GameSettings::gamefield[_address.Left()]->KillSquareNearEnergy();
	GameSettings::gamefield[_address.Up()]->KillSquareNearEnergy();
	GameSettings::gamefield[_address.Right()]->KillSquareNearEnergy();
	GameSettings::gamefield[_address.Down()]->KillSquareNearEnergy();

	Gadgets::cellWalls.RemoveChipWall(_address, Gadgets::CellWalls::DOWN | Gadgets::CellWalls::LEFT);
	Gadgets::cellWalls.RemoveChipWall(_address.Up(), Gadgets::CellWalls::DOWN);
	Gadgets::cellWalls.RemoveChipWall(_address.Right(), Gadgets::CellWalls::LEFT);

	if( !_is_source ) {
		GameField::Get()->StopEnergySourceEffects();
	}

	Gadgets::bears.Update();
	for(auto i : _snaps_listeners_on_energy_fill)
	{
		i.first->OnEnergySquareFilled(_address);
	}
	_snaps_listeners_on_energy_fill.clear();
}

bool Square::AddSnapListenerOnEnergyFill(SnapGadgetReleaseBorderElement* snap_element)
{
	if(_snaps_listeners_on_energy_fill.find(snap_element) != _snaps_listeners_on_energy_fill.end())
	{
		return false;
	}
	_snaps_listeners_on_energy_fill[snap_element] = true;
	return true;
}

void Square::DrawQuad(int quad)
{
	FPoint center = FPoint(_address.ToPoint()) * GameSettings::SQUARE_SIDEF + GameSettings::CELL_HALF;
	float r = GameSettings::SQUARE_SIDEF * 0.5f;
	float rs = r * 0.9f;
	if( quad == CENTER )
	{
		AddQuadToVB(vert_buffer, center +  Field::cp[12] * r, center +  Field::cp[13] * r, center, center +  Field::cp[14] * r);
		AddQuadToVB(vert_buffer, center +  Field::cp[14] * r, center +  Field::cp[15] * r, center, center +  Field::cp[0] * r);
		AddQuadToVB(vert_buffer, center, center +  Field::cp[0] * r, center +  Field::cp[2] * r, center +  Field::cp[1] * r);
		AddQuadToVB(vert_buffer, center, center +  Field::cp[2] * r, center +  Field::cp[4] * r, center +  Field::cp[3] * r);
		AddQuadToVB(vert_buffer, center +  Field::cp[6] * r, center, center +  Field::cp[5] * r, center +  Field::cp[4] * r);
		AddQuadToVB(vert_buffer, center +  Field::cp[8] * r, center, center +  Field::cp[7] * r, center +  Field::cp[6] * r);
		AddQuadToVB(vert_buffer, center +  Field::cp[9] * r, center +  Field::cp[10] * r, center +  Field::cp[8] * r, center);
		AddQuadToVB(vert_buffer, center +  Field::cp[11] * r, center +  Field::cp[12] * r, center +  Field::cp[10] * r, center);
	}
	else if( quad == LEFT_TOP )
	{
		FPoint corner = center + FPoint(-1.0f, 1.0f) * r;
		AddQuadToVB(vert_buffer, corner, center + Field::cp[0] * r, center + Field::cp[14] * rs, center + Field::cp[0] * rs);
		AddQuadToVB(vert_buffer, corner, center + Field::cp[14] * rs, center + Field::cp[12] * r, center + Field::cp[12] * rs);
	}
	else if( quad == LEFT_BOTTOM )
	{
		FPoint corner = center + FPoint(-1.0f, -1.0f) * r;
		AddQuadToVB(vert_buffer, corner, center + Field::cp[12] * r, center + Field::cp[10] * rs, center + Field::cp[12] * rs);
		AddQuadToVB(vert_buffer, corner, center + Field::cp[10] * rs, center + Field::cp[8] * r, center + Field::cp[8] * rs);
	}
	else if( quad == RIGHT_TOP )
	{
		FPoint corner = center + FPoint(1.0f, 1.0f) * r;
		AddQuadToVB(vert_buffer, corner, center + Field::cp[4] * r, center + Field::cp[2] * rs, center + Field::cp[4] * rs);
		AddQuadToVB(vert_buffer, corner, center + Field::cp[2] * rs, center + Field::cp[0] * r, center + Field::cp[0] * rs);
	}
	else if( quad == RIGHT_BOTTOM )
	{
		FPoint corner = center + FPoint(1.0f, -1.0f) * r;
		AddQuadToVB(vert_buffer, corner, center + Field::cp[8] * r, center + Field::cp[6] * rs, center + Field::cp[8] * rs);
		AddQuadToVB(vert_buffer, corner, center + Field::cp[6] * rs, center + Field::cp[4] * r, center + Field::cp[4] * rs);
	}
}

void Square::Draw()
{
	if( !Game::visibleRect.Contains(_address.ToPoint()) )
		return;

	FPoint squarePos = FPoint(_address.ToPoint()) * GameSettings::SQUARE_SIDEF;
	if( TotalParts() >= max_parts_total)
	{
		// полная клетка, просто рисуем квадрат
		FRect drawRect(0.0f, GameSettings::SQUARE_SIDEF, 0.0f, GameSettings::SQUARE_SIDEF);
		drawRect.MoveTo(squarePos);
		AddQuadToVB(vert_buffer, drawRect.LeftBottom(), drawRect.LeftTop(), drawRect.RightBottom(), drawRect.RightTop());
	}
	else
	{
		bool quadDrawn[TOTAL_SEGMENTS];
		for(int i = 0; i < TOTAL_SEGMENTS; ++i )
			quadDrawn[i] = false;
		if( FullOfEnergy(CENTER) && FullOfEnergy(LT_INNER) && FullOfEnergy(LB_INNER) && FullOfEnergy(RT_INNER) && FullOfEnergy(RB_INNER) ) {
			DrawQuad(CENTER);
			quadDrawn[CENTER] = true;
		}
		if( FullOfEnergy(LEFT_TOP) && FullOfEnergy(LT_INNER) ) {
			DrawQuad(LEFT_TOP);
			quadDrawn[LEFT_TOP] = true;
		}
		if( FullOfEnergy(LEFT_BOTTOM) && FullOfEnergy(LB_INNER) ) {
			DrawQuad(LEFT_BOTTOM);
			quadDrawn[LEFT_BOTTOM] = true;
		}
		if( FullOfEnergy(RIGHT_TOP) && FullOfEnergy(RT_INNER) ) {
			DrawQuad(RIGHT_TOP);
			quadDrawn[RIGHT_TOP] = true;
		}
		if( FullOfEnergy(RIGHT_BOTTOM) && FullOfEnergy(RB_INNER) ) {
			DrawQuad(RIGHT_BOTTOM);
			quadDrawn[RIGHT_BOTTOM] = true;
		}

		if( _parts )
		{
			IPoint offset = _address.ToPoint() * ENERGY_PARTS_SQUARE_SIDE;

			for(int i = 0; i < ENERGY_PARTS_SQUARE_SIDE; ++i)
			{
				for(int j = 0; j < ENERGY_PARTS_SQUARE_SIDE; ++j)
				{
					Part &p = getPart(i,j);
					if( p.e > 0.0f && !quadDrawn[PartQuadrant(i,j)])
					{
						FRect drawRect(0.0f, GameSettings::SQUARE_SIDEF / ENERGY_PARTS_SQUARE_SIDE, 0.0f, GameSettings::SQUARE_SIDEF / ENERGY_PARTS_SQUARE_SIDE);
						drawRect.MoveTo(squarePos);
						FPoint pos = FPoint(i, j) * (GameSettings::SQUARE_SIDEF / ENERGY_PARTS_SQUARE_SIDE);
						drawRect.MoveBy(pos);

						float en[9]; // энергии в соседних частицах
						// 012
						// 345
						// 678
						en[0] = field.getEnergy(offset.x + i - 1, offset.y + j + 1);
						en[1] = field.getEnergy(offset.x + i,     offset.y + j + 1);
						en[2] = field.getEnergy(offset.x + i + 1, offset.y + j + 1);
						en[3] = field.getEnergy(offset.x + i - 1, offset.y + j);
						en[4] = field.getEnergy(offset.x + i,     offset.y + j);
						en[5] = field.getEnergy(offset.x + i + 1, offset.y + j);
						en[6] = field.getEnergy(offset.x + i - 1, offset.y + j - 1);
						en[7] = field.getEnergy(offset.x + i,     offset.y + j - 1);
						en[8] = field.getEnergy(offset.x + i + 1, offset.y + j - 1);

						float e1 = std::min(std::min(en[0], en[1]), std::min(en[3], en[4]));
						float e2 = std::min(std::min(en[1], en[2]), std::min(en[4], en[5]));
						float e3 = std::min(std::min(en[3], en[4]), std::min(en[6], en[7]));
						float e4 = std::min(std::min(en[4], en[5]), std::min(en[7], en[8]));

						DrawMarchingSquare(vert_buffer, drawRect, e1, e2, e3, e4, Settings::energyThreshold);

						float et = std::min(0.5f, Settings::energyThreshold);
						e1 = (e1 > et) ? (1.0f - e1) : e1;
						e2 = (e2 > et) ? (1.0f - e2) : e2;
						e3 = (e3 > et) ? (1.0f - e3) : e3;
						e4 = (e4 > et) ? (1.0f - e4) : e4;
						DrawMarchingSquare(vb_front, drawRect, e1, e2, e3, e4, Settings::frontThreshold, Settings::frontColor, ZBuf::Z_EPS);
					}
				}
			}
		}
	}
}

bool Square::FullOfEnergy(int quad) const
{
	return _parts_num[quad] >= max_parts_quad[quad];
}

bool Square::EnergyExists(int quad, const int value) const
{
	return _parts_num[quad] >= value;
}

int Square::TotalParts() const
{
	int total = 0;
	for(int p : _parts_num) {
		total += p;
	}
	return total;
}



Field::Field()
	: bufEnergySquare(nullptr)
	, _energy_flowing(false)
	, _update_buffers(true)
	, _energyMoveTrackId(0)
{
	bufEnergySquare = new Square( Game::FieldAddress(-1,-1) );

	// Инициализируем константы общего кол-ва частиц в клетке и ее зонах
	for(int i = 0; i < Square::TOTAL_SEGMENTS; ++i)
		max_parts_quad[i] = 0;

	for(int i = 0; i < ENERGY_PARTS_SQUARE_SIDE; ++i)
	{
		for(int j = 0; j < ENERGY_PARTS_SQUARE_SIDE; ++j)
		{
			int q = PartQuadrant(i,j);
			max_parts_quad[q]++;
		}
	}
	max_parts_total = ENERGY_PARTS_SQUARE_SIDE * ENERGY_PARTS_SQUARE_SIDE;

	//Пересчитаем точки окружности на чуть более выпуклые.
	for(size_t i = 0; i < 16; i++)
	{
		float t = i/16.f;
		float angle = math::PI*0.5f - t*math::PI*2.f;
		float c = math::cos(angle);
		float s = math::sin(angle);
		cp[i] = FPoint(math::sign(c) * math::sqrt(math::abs(c)), math::sign(s) * math::sqrt(math::abs(s)));
	}
}

Field::~Field()
{
	delete bufEnergySquare;
}

void Field::Init()
{
	// Инициализируем клетки с энергией
	_squares.Init(GameSettings::FIELD_MAX_WIDTH, GameSettings::FIELD_MAX_HEIGHT, bufEnergySquare, 0, 0);
	for(int i = 0; i < _squares.Width(); ++i)
	{
		for(int j = 0; j < _squares.Height(); ++j)
		{
			Game::FieldAddress fa(i,j);
			Game::Square *sq = GameSettings::gamefield[fa];
			if( !Game::isBuffer(sq) || !Game::isBuffer(GameSettings::gamefield[fa.Left()]) 
			                        || !Game::isBuffer(GameSettings::gamefield[fa.Right()])
									|| !Game::isBuffer(GameSettings::gamefield[fa.Up()])
									|| !Game::isBuffer(GameSettings::gamefield[fa.Down()])
									|| !Game::isBuffer(GameSettings::gamefield[fa.Shift(1,1)]) 
			                        || !Game::isBuffer(GameSettings::gamefield[fa.Shift(-1,1)])
									|| !Game::isBuffer(GameSettings::gamefield[fa.Shift(-1,-1)])
									|| !Game::isBuffer(GameSettings::gamefield[fa.Shift(1,-1)]))
			{
				Square *esq = new Square(fa);
				_squares[fa] = esq;
				esq->UpdateWalls();
				_squaresv.push_back(esq);
			}
		}
	}
	SetEnergyFlowing(false);
	_update_buffers = true;

	_frontParticleScale.Clear();
	_frontParticleScale.addKey(0.45f);
	_frontParticleScale.addKey(1.0f);
	_frontParticleScale.addKey(0.3f);
	_frontParticleScale.addKey(0.07f);
	_frontParticleScale.CalculateGradient();

	_frontParticleAlpha.Clear();
	_frontParticleAlpha.addKey(0.8f);
	_frontParticleAlpha.addKey(1.0f);
	_frontParticleAlpha.addKey(0.95f);
	_frontParticleAlpha.addKey(0.8f);
	_frontParticleAlpha.addKey(0.0f);
	_frontParticleAlpha.CalculateGradient();

	Front::particles.resize( Front::particleTypes.size() );
}

void Field::Release()
{
	for(Square* sq : _squaresv) {
		delete sq;
	}
	_squaresv.clear();

	_squares.Release();

	Front::particles.clear();

	vert_buffer.clear();
	vb_front.clear();
}

float Field::getEnergy(int i, int j) const
{
	if( i < 0 || i >= GameSettings::FIELD_MAX_WIDTH * ENERGY_PARTS_SQUARE_SIDE
	 || j < 0 || j >= GameSettings::FIELD_MAX_HEIGHT * ENERGY_PARTS_SQUARE_SIDE )
	{
		return 0.0f;
	} else {
		IPoint saddr(i / ENERGY_PARTS_SQUARE_SIDE, j / ENERGY_PARTS_SQUARE_SIDE);
		IPoint paddr(i % ENERGY_PARTS_SQUARE_SIDE, j % ENERGY_PARTS_SQUARE_SIDE);
		return _squares[saddr]->getEnergy(paddr.x, paddr.y);
	}
}

bool Field::FullOfEnergy(Game::FieldAddress address, int quad) const
{
	Square *sq = _squares[address];
	return sq && sq->FullOfEnergy(quad);
}

bool Field::EnergyExists(Game::FieldAddress address, int quad) const
{
	Square *sq = _squares[address];
	return sq && sq->EnergyExists(quad);
}

bool Field::EnergyIsFlowing() const
{
	return _energy_flowing;
}

void Field::SetEnergyFlowing(bool value)
{
	if (_energy_flowing != value)
	{
		_energy_flowing = value;
		if (_energy_flowing)
		{
			//если начала течь, играем
			_energyMoveTrackId = MM::manager.PlaySample("EnergyMove",true);
		}
		else
		{
			//если остановилась, молчим
			if (_energyMoveTrackId)
			{
				MM::manager.StopSample(_energyMoveTrackId);
				_energyMoveTrackId = 0;
			}
			Tutorial::luaTutorial.AcceptMessage( Message("OnEnergyStop") );
		}
	}
}

void Field::Update(float dt)
{
	bool energy_flowing = false;

	for(Square *sq : _squaresv)
	{
		energy_flowing |= sq->Update(dt);
	}

	for(auto &particleList : Front::particles)
	{
		for(auto itr = particleList.begin(); itr != particleList.end(); )
		{
			itr->local_time += dt / itr->lifetime;
			if(itr->local_time >= 1.0f) {
				itr = particleList.erase(itr);
			} else {
				++itr;
			}
		}
	}

	SetEnergyFlowing(energy_flowing);
}

void Field::Draw()
{
	bool need_update = _energy_flowing || GameField::Get()->_fieldMoving;
	if(_update_buffers || need_update)
	{
		vert_buffer.clear();
		vb_front.clear();
		for(Square *sq : _squaresv)
		{
			sq->Draw();
		}
	}
	_update_buffers = need_update;

	if( !vert_buffer.empty() )
	{
		VertexBuffer vb;
		vb.Init(vert_buffer.size());
		vb.SetRawData(0, &vert_buffer[0], vert_buffer.size());
		vb.Upload();
		vb.Draw();
	}
}

void Field::DrawFront()
{
	if( !vb_front.empty() )
	{
		VertexBuffer vb;
		vb.Init(vb_front.size());
		vb.SetRawData(0, &vb_front[0], vb_front.size());
		vb.Upload();
		vb.Draw();
	}
}

void Field::DrawParticles()
{
	Render::device.PushMatrix();
	Render::device.MatrixTranslate(math::Vector3(0.0f, 0.0f, ZBuf::ENERGY - ZBuf::Z_EPS));

	for(size_t type = 0; type < Front::particles.size(); type++)
	{
		std::list<Front::Particle> &particleList = Front::particles[type];
		const Front::ParticlePreset &preset = Front::particleTypes[type];

		if( !particleList.empty() )
		{
			VertexBuffer vb;
			vb.InitQuadBuffer(particleList.size());
			int idx = 0;

			for(Front::Particle &p : particleList )
			{
				float size = _frontParticleScale.getGlobalFrame(p.local_time) * p.size;
				FPoint p1 = FPoint(0, size).Rotated(p.angle);
				FPoint p2(p1.y, -p1.x);

				float alpha = math::clamp(0.0f, 1.0f, _frontParticleAlpha.getGlobalFrame(p.local_time) * p.alpha);
				Color col = preset.color;
				col.alpha = math::round(255.0f * alpha);

				vb.SetQuad(idx++, p.pos - p1 - p2, p.pos - p1 + p2, p.pos + p1 - p2, p.pos + p1 + p2, col);
			}

			Render::device.SetBlendMode(preset.blend);
			preset.tex->Bind();
			
			vb.Upload();
			vb.Draw();
		}
	}

	Render::device.SetBlendMode(Render::ALPHA);
	Render::device.PopMatrix();
}

void Field::UpdateSquare(Game::FieldAddress addr)
{
	if(_squares.Height() > 0 && _squares.Width() > 0) // иногда массив клеток может быть пустым (в редакторе например), проверяем этот случай
	{
		_update_buffers |= _squares[addr]->UpdateWalls();
		_update_buffers |= _squares[addr.Left()]->UpdateWalls();
		_update_buffers |= _squares[addr.Right()]->UpdateWalls();
		_update_buffers |= _squares[addr.Up()]->UpdateWalls();
		_update_buffers |= _squares[addr.Down()]->UpdateWalls();
		_update_buffers |= _squares[addr.Shift(-1,-1)]->UpdateWalls();
		_update_buffers |= _squares[addr.Shift(-1,1)]->UpdateWalls();
		_update_buffers |= _squares[addr.Shift(1,-1)]->UpdateWalls();
		_update_buffers |= _squares[addr.Shift(1,1)]->UpdateWalls();
	}
}

void Field::StartEnergyInSquare(Game::FieldAddress addr)
{
	_squares[addr]->StartEnergySource();
}

bool Field::CanFlowFromTo(Game::FieldAddress from, Game::FieldAddress to) const
{
	IPoint off = to.ToPoint() - from.ToPoint();
	Assert( math::abs(off.x) <= 1 );
	Assert( math::abs(off.y) <= 1 );
	Assert( math::abs(off.x) + math::abs(off.y) > 0 );

	const Square *sq_from = _squares[from];
	const Square *sq_to = _squares[to];

	return sq_from->EnergyExists(Square::CENTER, 0) && sq_to->EnergyExists(Square::CENTER, 0);

	// TODO: учет стен
}

float Field::CalcDistanceToFlow() const
{
	std::map<Game::FieldAddress, float> dist_map;

	for(Square *sq : _squaresv ) {
		if( sq->FullOfEnergy() )
			dist_map.insert( std::make_pair(sq->getAddress(), 0.0f) );
	}

	bool flowing = true;
	while( flowing )
	{
		flowing = false;
		for(auto itr = dist_map.begin(); itr != dist_map.end(); ++itr )
		{
			for(size_t k = 0; k < Gadgets::checkDirsInfo.count; ++k) {
				IPoint off = Gadgets::checkDirsInfo[k];
				Game::FieldAddress to = itr->first.Shift(off.x, off.y);
				if( CanFlowFromTo(itr->first, to) )
				{
					float dist = (off.x == 0 || off.y == 0) ? 1.0f : 1.3f;
					auto result = dist_map.insert( std::make_pair(to, itr->second + dist) );
					if( result.second ) {
						flowing = true;
					} else {
						result.first->second = std::min(result.first->second, itr->second + dist);
					}
				}
			}
		}
	}

	float distance = 0.0f;
	for(auto &val : dist_map) {
		distance = std::max(distance, val.second);
	}

	return distance;
}

bool Field::AddSnapListenerOnEnergyFill(Game::FieldAddress address, SnapGadgetReleaseBorderElement* snap_element)
{
	Square *sq = _squares[address];
	return sq && sq->AddSnapListenerOnEnergyFill(snap_element);
}

} // namespace Energy
