#include "stdafx.h"
#include "Place2D.h"
#include "Game.h"
#include "SomeOperators.h"

namespace Place2D
{
	//Радиусы закругления
	float R_IN = 0.5f; //внутренний
	float R_OUT = 0.5f; //внешний
	float R_OUT2 = 0.5f; //внешний2

	bool can_draw_second_quad = true; //Рисовать инвертированные углы в клетках светлыми (первый квадрат)

	FPoint D_INFLATE = FPoint(0.f, 0.f); //Увеличение квадрата отрисовки (для не прозрачных тексур, заливок!)
	float D_INFLATE_PARAM = 0.f;
	IRect view_rect;

	void SetParams(float r_in, float r_out, float r_out2, float d_inflate)
	{
		R_IN = r_in;
		R_OUT = r_out2;
		R_OUT2 = r_out;
		D_INFLATE = FPoint(d_inflate, d_inflate);
		D_INFLATE_PARAM = d_inflate/GameSettings::SQUARE_SIDEF;
	}

	void SetViewRect(IRect rect)
	{
		view_rect = rect;
	}

	bool up_element(Game::Square *sq)
	{
		return sq->GetWood() > 0 || sq->IsIce() || sq->IsStone();
	}


	void SetChessOnConers(const bool value)
	{
		can_draw_second_quad = value;
	}

	void ScopePiece::Load(rapidxml::xml_node<> *xml_element, Render::Texture *texture, float fscale)
	{
		rapidxml::xml_node<> *xml_frect = xml_element->first_node("frect");

		bool type_and = Xml::GetStringAttributeOrDef(xml_element, "type", "or") == "and";
		zDepthOffset = Xml::GetFloatAttributeOrDef(xml_element, "zDepthOffset", 0.f);
		int variant = Xml::GetIntAttributeOrDef(xml_element, "variant", 0);
		ScopeRect scope_rect;

		scope_rect.frect = xml_frect;
		scope_rect.frect.xStart *= fscale/texture->Width();
		scope_rect.frect.xEnd *= fscale/texture->Width();
		scope_rect.frect.yStart *= fscale/texture->Height();
		scope_rect.frect.yEnd *= fscale/texture->Height();


		Render::CheckUV(scope_rect.frect);

		rapidxml::xml_node<> *xml_irect = xml_element->first_node("irect");
		if(xml_irect)
		{
			float x = Xml::GetFloatAttribute(xml_irect, "x");
			float y = Xml::GetFloatAttribute(xml_irect, "y");
			float w = Xml::GetFloatAttribute(xml_irect, "width");
			float h = Xml::GetFloatAttribute(xml_irect, "height");
			scope_rect.rect = FRect(x, x+w, y, y + h).Scaled(GameSettings::SQUARE_SIDEF);
		}else{
			scope_rect.rect = FRect(0.f, GameSettings::SQUARE_SIDEF, 0.f, GameSettings::SQUARE_SIDEF).MovedBy(-GameSettings::CELL_HALF);
		}

		texture->TranslateUV(scope_rect.rect, scope_rect.frect);

		if(type_and)
		{
			and_rects.push_back(scope_rect);
		}else{
			if(variant == 0)
			{
				or_rects_v0.push_back(scope_rect);
			}else{
				or_rects_v1.push_back(scope_rect);
			}
		}
	}

	ScopePieceForDraw::ScopePieceForDraw(FRect frect_, FRect rect_, float z_)
		: frect(frect_)
		, rect(rect_)
		, z(z_)
	{
	}


	std::vector<ScopePieceForDraw> *current_borders = NULL;

	std::map<std::string, BorderInfo> borders;

	std::vector<ScopePieceForDraw> wall_borders, ice_borders, stone_borders, wood_borders, wall_energy_borders, misc_borders;

	BorderInfo *mask_map = NULL;

	VertexBufferIndexed *buffer=NULL;

	std::vector<std::vector<CellInfo> > _map;
	Color current_draw_color = Color::WHITE;

	struct DestructorChecker{
		~DestructorChecker()
		{
			MyAssert(Place2D::buffer == NULL); //Небыл вызван Place2D::Release();
		}
	}destructor_checker;

	CellInfo::CellInfo()
		: is_exist(false)
		, visible(false)
		, exist_count(0)
		, visible_count(0)
		, wall_exist(false)
		, with_up(false)
	{
	}

	void BorderInfo::Load(rapidxml::xml_node<> *xml_info)
	{
		texture = Core::resourceManager.Get<Render::Texture>(Xml::GetStringAttribute(xml_info, "texture"));
		rapidxml::xml_node<> *xml_scope = xml_info->first_node("item");
		float fscale = Xml::GetFloatAttributeOrDef(xml_info, "fscale", 1.f);
		while(xml_scope)
		{
			BYTE mask = Xml::GetIntAttribute(xml_scope, "mask");
			map[mask].Load(xml_scope, texture, fscale);
			xml_scope = xml_scope->next_sibling("item");
		}
	}

	void Init(rapidxml::xml_node<> *xml_root)
	{
		Place2D::borders.clear();
		//ToDo края инициализируются немного не совсем логично, можно переработать
		rapidxml::xml_node<> *xml_borders = xml_root->first_node("Borders");
		rapidxml::xml_node<> *xml_info = xml_borders->first_node("info");
		while(xml_info)
		{
			std::string name = Xml::GetStringAttribute(xml_info, "name");
			Place2D::borders[name].Load(xml_info);
			xml_info = xml_info->next_sibling("info");
		}
		Place2D::buffer = new VertexBufferIndexed();
		Place2D::buffer->isStatic = true; //c false проблемы
		//Place2D::mask_map = map;
		_map.resize(MAP_WIDTH);
		for(size_t i = 0; i < MAP_WIDTH; i++)
		{
			_map[i].resize(MAP_HEIGHT);
		}	
		Place2D::SetParams(0.5f, 0.5f, 0.5f);
	}

	void Release()
	{
		if(Place2D::buffer)
		{
			delete Place2D::buffer;
			Place2D::buffer = NULL;
		}else{
			MyAssert(false);
		}
	}

	void AddBorder(const FPoint &pos, BYTE mask, float zDepth, bool variant_second = 0)
	{
		if(mask_map->map.find(mask) == mask_map->map.end())
		{
			return;
		}
		if(current_borders)
		{
			ScopePiece &orig = mask_map->map[mask];
			if(!orig.or_rects_v0.empty())
			{
				std::vector<ScopeRect>* or_rects = &orig.or_rects_v0;
				if(variant_second && !orig.or_rects_v1.empty())
				{
					or_rects = &orig.or_rects_v1;
				}

				//На основании позиции эмулируем рандом для выбора границы
				size_t variant = 0;
                
				if(or_rects->size() > 1)
				{
					variant = (int(sqrt(abs(pos.x*pos.y))) >> 3) % or_rects->size();
				}
				ScopeRect &scope_rect = (*or_rects)[variant];
				current_borders->push_back(ScopePieceForDraw(scope_rect.frect, scope_rect.rect.MovedBy(pos), zDepth));
			}
			//Теперь занесем все обязательные для отрисовки элементы
			size_t count = orig.and_rects.size();
			for(size_t i = 0; i < count; i++)
			{
				current_borders->push_back(ScopePieceForDraw(orig.and_rects[i].frect, orig.and_rects[i].rect.MovedBy(pos), zDepth));
			}
		}else{
			//MyAssert(false);
		}
	}

	void ClearDrawBuffer()
	{
		Place2D::buffer->Reinit(0, 0);
		Place2D::buffer->_ibuffer.clear();
		Place2D::buffer->_buffer.clear();
	}

	void DrawBuffer()
	{
		size_t count_indexes = Place2D::buffer->_ibuffer.size();
		if(count_indexes > 0)
		{
			Place2D::buffer->numIndices = count_indexes;
			Place2D::buffer->numVertices = Place2D::buffer->_buffer.size();
			MyAssert(Place2D::buffer->numVertices > 0);
			Place2D::buffer->UploadVertex();
			Place2D::buffer->UploadIndex();
			Place2D::buffer->Draw();
			ClearDrawBuffer();
		}
	}

	const float INFLATE_PARAM = 4.f;
	const IPoint dirs_ccw[4] =  { IPoint(1,1), IPoint(0,1), IPoint(0,0), IPoint(1,0) };

	void DrawFan(const FPoint &pos, const FPoint &p0, const FPoint &v_center, float angle, const IRect &rectDraw, float zDepth, float r, size_t k, bool can_draw_second_uv)
	{
		//Начальные точки
		DWORD id_first = Place2D::buffer->_buffer.size();

		FPoint uv_start, uv;
		if(can_draw_second_uv)
		{
			uv_start = p0/ rectDraw;
		}else{
			uv_start.x = 0.25f;
			uv_start.y = 0.25f;
		}
		Place2D::buffer->_buffer.push_back(Render::QuadVert(p0.x, p0.y, zDepth, Place2D::current_draw_color, uv_start.x, uv_start.y));
		
		size_t step_count_descret = 7;
		float range = step_count_descret + 0.f;
		for(size_t i = 0; i <= step_count_descret; i++)
		{
			FPoint dir = v_center + FPoint(r, 0.f).Rotated(math::lerp(angle, angle+math::PI*0.5f, i/range));
			FPoint p1(pos.x + dir.x*GameSettings::SQUARE_SIDEF, pos.y + dir.y*GameSettings::SQUARE_SIDEF);
			if(can_draw_second_uv)
			{
				uv = p1/ rectDraw;
			}else{
				//Рисуем центром первого квадрата!
				uv.x = 0.25f;
				uv.y = 0.25f;
			}
			Place2D::buffer->_buffer.push_back(Render::QuadVert(p1.x, p1.y, zDepth, Place2D::current_draw_color, uv.x, uv.y));
		}
		for(size_t i = 1; i <= step_count_descret; i++)
		{
			Place2D::buffer->_ibuffer.push_back(id_first); 
			Place2D::buffer->_ibuffer.push_back(id_first + i);
			Place2D::buffer->_ibuffer.push_back(id_first + i + 1);
		}
	}


	void DrawCircle4(const FPoint &pos, const IRect &rectDraw, float zDepth, size_t k, bool invert, bool add_border, bool border_variant_second = false) //r - относительно 1 == Ширине клетки
	{
		float angle = k*math::PI*0.5f;
		FPoint v0(0.f, 0.f); 
		float r = invert ? Place2D::R_IN : Place2D::R_OUT;
		if(border_variant_second)
		{
			r = Place2D::R_OUT2;
		}
		//r += Place2D::D_INFLATE_PARAM;
		FPoint v_real_center(0.5f, 0.5f);
		v_real_center.Rotate(angle);
		FPoint v_center_fan(r, r);
		v_center_fan.Rotate(angle);
		if(!invert)
		{
			v0 = v_center_fan;
		}
		FPoint p0 = pos + v0*GameSettings::SQUARE_SIDEF;
		
		bool can_draw_second_uv = Place2D::can_draw_second_quad || !invert;

		DrawFan(pos, p0, v_center_fan, angle + math::PI, rectDraw, zDepth, r, k, can_draw_second_uv);
		if(!invert)
		{
			// Рисуется недостающая область после частичного закругления. То что не может отрисовать Fan.
			// Чтобы понять что рисуется закоментируйте
			if(r < 0.5f)
			{
				float r_plus = r*1.f; //Рисуем немного больше чем нужно. Наезжаем на fan.

				FPoint v0 = pos + v_center_fan*GameSettings::SQUARE_SIDEF;//Рисуем немного больше чем нужно. Наезжаем на fan.
				FPoint v1 = pos + FPoint(r_plus, 0.f).Rotated(angle) * GameSettings::SQUARE_SIDEF;
				FPoint v2 = pos + FPoint(0.5f, 0.f).Rotated(angle) * GameSettings::SQUARE_SIDEF; 
				FPoint v3 = pos + v_real_center*GameSettings::SQUARE_SIDEF;
				FPoint v4 = pos + FPoint(0.5f, 0.f).Rotated(angle + math::PI*0.5f) * GameSettings::SQUARE_SIDEF;
				FPoint v5 = pos + FPoint(r_plus, 0.f).Rotated(angle + math::PI*0.5f) * GameSettings::SQUARE_SIDEF;
				
				FPoint uv0, uv1, uv2, uv3, uv4, uv5;

				if(can_draw_second_uv)
				{
					uv0 = v0/rectDraw;
					uv1 = v1/rectDraw;
					uv2 = v2/rectDraw;
					uv3 = v3/rectDraw;
					uv4 = v4/rectDraw;
					uv5 = v5/rectDraw;
				}else{
					uv0 = uv1 = uv2 = uv3 = uv4 = uv5 = FPoint(0.25f, 0.25f);
				}

				DWORD id_first = Place2D::buffer->_buffer.size();

				Place2D::buffer->_buffer.push_back(Render::QuadVert(v0.x, v0.y, zDepth, Place2D::current_draw_color, uv0.x, uv0.y));
				Place2D::buffer->_buffer.push_back(Render::QuadVert(v1.x, v1.y, zDepth, Place2D::current_draw_color, uv1.x, uv1.y));
				Place2D::buffer->_buffer.push_back(Render::QuadVert(v2.x, v2.y, zDepth, Place2D::current_draw_color, uv2.x, uv2.y));
				Place2D::buffer->_buffer.push_back(Render::QuadVert(v3.x, v3.y, zDepth, Place2D::current_draw_color, uv3.x, uv3.y));
				Place2D::buffer->_buffer.push_back(Render::QuadVert(v4.x, v4.y, zDepth, Place2D::current_draw_color, uv4.x, uv4.y));
				Place2D::buffer->_buffer.push_back(Render::QuadVert(v5.x, v5.y, zDepth, Place2D::current_draw_color, uv5.x, uv5.y));

				for(size_t i = 1; i <= 4;i++)
				{
					Place2D::buffer->_ibuffer.push_back(id_first); 
					Place2D::buffer->_ibuffer.push_back(id_first + i);
					Place2D::buffer->_ibuffer.push_back(id_first + i + 1);
				}

			}
		}
		if(Place2D::mask_map && add_border)
		{
			BYTE mask = 1 << k;
			if(invert)
			{
				mask = mask ^ 0x0F;
			}
			AddBorder(pos, mask, zDepth, border_variant_second);
		}
	}

	void Calculate(bool is_wall(Game::Square *sq), bool is_visible(Game::Square *sq))
	{
		IPoint offset = Place2D::view_rect.LeftBottom();
		for (int x = Place2D::view_rect.Width(); x >=0; x--)
		{
			for (int y = Place2D::view_rect.Height(); y >=0; y--)
			{
				Game::Square *sq = GameSettings::gamefield[offset.x + x + 1][offset.y + y +1];
			
				MyAssert(x < MAP_HEIGHT && y < MAP_WIDTH);
				CellInfo &cell = _map[x][y];
				cell.pos =  IPoint(offset.x + x +1, offset.y + y+1) * GameSettings::SQUARE_SIDE;

				cell.visible = is_visible(sq);
				cell.is_exist = cell.visible && is_wall(sq);
				cell.wall_exist = sq->GetWall() > 0 || !Game::isVisible(sq);

				if(!Place2D::can_draw_second_quad)
				{
					cell.with_up = up_element(sq);
				}

				cell.exist_count = 0;
				cell.visible_count = 0;
				for(size_t r = 0; r < 4; r++)
				{
					const IPoint &p_dir = dirs_ccw[r];
					MyAssert((x + p_dir.x < MAP_HEIGHT) && (y + p_dir.y < MAP_WIDTH));
					MyAssert((x + p_dir.x >= 0 ) && (y + p_dir.y >= 0));
					CellInfo &cell_dir = _map[x + p_dir.x][y + p_dir.y];
					if(cell_dir.is_exist){
						cell.exist_count++;
					}
					if(cell_dir.visible){
						cell.visible_count++;
					}
					for(size_t k = 0; k < 4; k++)
					{
						BYTE dir = (r + 4 - k) % 4;
						cell.coners[k][dir] = cell_dir.is_exist;
						cell.vis[k][dir] = cell_dir.visible;
						cell.wall_coner[k][dir] = cell_dir.wall_exist;
						cell.is_up[k][dir] = cell_dir.with_up;
					}
				}
				if(cell.exist_count < 3)
				{
					//Особая обработка земли рядом с границами
					for(size_t k = 0; k < 4; k++)
					{
						if(!cell.coners[k][0])
						{
							//инвертированый полукруг
							if(cell.vis[k][0])
							{
								if(cell.visible_count < 3 && (cell.exist_count == 1))									
								{
									cell.coners[k][1] = true;
									cell.coners[k][2] = true;
									cell.coners[k][3] = true;
								}else if(cell.exist_count == 2 && cell.visible_count == 3)
								{
									cell.coners[k][1] = true;
									cell.coners[k][2] = true;
									cell.coners[k][3] = true;
								}
							}else if(!cell.vis[k][0]){
								if(		(cell.coners[k][1] || cell.coners[k][3])
									&&  (cell.vis[k][1] && cell.vis[k][3])
								)
								{
									cell.coners[k][1] = true;
									cell.coners[k][2] = true;
									cell.coners[k][3] = true;
								}
							}
						}else if(cell.coners[k][0])
						{
							//полукруг
							if(cell.exist_count == 1 && cell.visible_count != 2 
								&& (cell.vis[k][1] == cell.vis[k][3])
								)
							{
								cell.coners[k][1] = false;
								cell.coners[k][3] = false;
							}else if(cell.exist_count == 2 && cell.visible_count >= 3
								&& (cell.coners[k][1] == cell.coners[k][3])
								)
							{
								cell.coners[k][1] = false;
								cell.coners[k][3] = false;
							}
						}
					}
				}
			}
		}
	}

	void DrawPlace(const Color &color, const IRect &rectDraw, float zDepth, bool need_draw_squares, bool need_draw_invert_corners)
	{
		Render::BeginColor(color);
		Place2D::current_draw_color = Render::device.GetCurrentColor();

		for (int y = Place2D::view_rect.Height()-1 ; y >= 0; y--)
		{
			for (int x = 0; x < Place2D::view_rect.Width(); x++)
			{
				MyAssert(x < MAP_HEIGHT && y < MAP_WIDTH);
				CellInfo &cell = _map[x][y];
				if(cell.visible_count == 0)
				{
					continue;
				}

				IRect rect_full(int(cell.pos.x)+GameSettings::SQUARE_SIDE/2, int(cell.pos.y)+GameSettings::SQUARE_SIDE/2, GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);

				for(size_t k = 0; k < 4; k++)
				{
					if(cell.coners[k][0] && (cell.coners[k][1] || cell.coners[k][3] || !cell.vis[k][1] || !cell.vis[k][3]) && cell.visible_count != 1)
					{
						FPoint v1(dirs_ccw[k].x*0.5f, dirs_ccw[k].y*0.5f);
						FPoint v0(v1.x - 0.5f,  v1.y - 0.5f);

						FPoint p0(cell.pos + v0*GameSettings::SQUARE_SIDEF);
						FPoint p1(cell.pos + v1*GameSettings::SQUARE_SIDEF);

						if(D_INFLATE.x != 0)
						{
							p0 -= D_INFLATE; //Немного расширили
							p1 += D_INFLATE;
						}

						FPoint uv0, uv1;
						if(!Place2D::can_draw_second_quad && cell.is_up[k][0])
						{
							uv0.x = uv1.x = 0.25f;
							uv0.y = uv1.y = 0.25f;
						}else{
							uv0 = p0/ rectDraw;
							uv1 = p1/ rectDraw;
						}

						DWORD start_index = Place2D::buffer->_buffer.size();

						Place2D::buffer->_buffer.push_back(Render::QuadVert(p0.x, p0.y, zDepth, Place2D::current_draw_color, uv0.x, uv0.y));
						Place2D::buffer->_buffer.push_back(Render::QuadVert(p1.x, p0.y, zDepth, Place2D::current_draw_color, uv1.x, uv0.y));
						Place2D::buffer->_buffer.push_back(Render::QuadVert(p0.x, p1.y, zDepth, Place2D::current_draw_color, uv0.x, uv1.y));
						Place2D::buffer->_buffer.push_back(Render::QuadVert(p1.x, p1.y, zDepth, Place2D::current_draw_color, uv1.x, uv1.y));
						
						Place2D::buffer->_ibuffer.push_back(start_index);
						Place2D::buffer->_ibuffer.push_back(start_index + 1);
						Place2D::buffer->_ibuffer.push_back(start_index + 2);

						Place2D::buffer->_ibuffer.push_back(start_index + 2);
						Place2D::buffer->_ibuffer.push_back(start_index + 1);
						Place2D::buffer->_ibuffer.push_back(start_index + 3);
					}
					else if(cell.coners[k][0])
					{
						if(!cell.coners[k][1] && !cell.coners[k][3])
						{
							//полукруг, есть возможность рисовать два варианта выпуклых радиусов cell.coners[k][2]
							DrawCircle4(cell.pos, rectDraw, zDepth, k, false, true, !cell.wall_coner[k][1] && !cell.wall_coner[k][3]);
						}
					}
					else if(need_draw_invert_corners && (!cell.coners[k][0] || !cell.vis[k][0]))
					{
						if(cell.coners[k][1] && cell.coners[k][2] && cell.coners[k][3])
						{
							//Уголок - круг
							DrawCircle4(cell.pos, rectDraw, zDepth, k, true, cell.vis[k][0]);
						}
					}
				}
				if(Place2D::mask_map && cell.visible_count >= 3)
				{
					BYTE mask = 0;
					int count = 0;
					for(size_t i = 0; i < 4; i++)
					{
						if(cell.coners[i][0] || !cell.vis[i][0])
						{
							mask = mask | (1 << i);
							count++;
						}
					}
					//Добавляем прямое ребро
					if(count == 2)
					{
						AddBorder(cell.pos, mask, zDepth);
					}
				}
			}
		}
		Render::EndColor();
		if(need_draw_squares)
		{
			DrawBuffer();
		}else{
			//Рисовать не надо. Набили буфер границ. Буфер для отрисовки очищаем.
			ClearDrawBuffer();
		}
	}

	void DrawBorders(std::vector<Place2D::ScopePieceForDraw> &vec, const std::string &border_id, Color color, bool clear)
	{
		size_t count = vec.size();
		if( count > 0 )
		{
			Place2D::borders[border_id].texture->Bind();
			VertexBuffer vb;
			vb.InitQuadBuffer(count);
			for(size_t i = 0; i < count; i++)
			{
				FRect rect = vec[i].rect;
				FRect frect = vec[i].frect;
				float z = vec[i].z - ZBuf::Z_EPS;
				vb.SetQuad(i, 
					math::Vector3(rect.xStart , rect.yStart, z), 
					math::Vector3(rect.xEnd, rect.yStart, z), 
					math::Vector3(rect.xStart , rect.yEnd, z), 
					math::Vector3(rect.xEnd, rect.yEnd, z),
					color,
					frect.xStart, frect.xEnd, frect.yStart, frect.yEnd);
			}
			if(clear)
			{
				vec.clear();
			}
			vb.Upload();
			vb.Draw();
		}
	}

	IPoint field_offset;

	void Clear()
	{
		current_borders = NULL;
		field_offset = Place2D::view_rect.LeftBottom();
		for(int i = 0; i < Place2D::view_rect.width; i++){
			for(int j = 0; j < Place2D::view_rect.height; j++){
				if(i < MAP_WIDTH && j < MAP_HEIGHT)
				{
					_map[i][j] = CellInfo();
				}else{
					Log::log.WriteWarn("Place2D::Clear offset("+ utils::lexical_cast(field_offset.x) + "," + utils::lexical_cast(field_offset.y) + ") i=" + utils::lexical_cast(i) + " j=" + utils::lexical_cast(j));
				}
			}
		}
		//Place2D::buffer->Init(0);
	}

	void AddAddress(const Game::FieldAddress &address)
	{
		IPoint pos = address.ToPoint() - field_offset;
		if(pos.x >= Place2D::view_rect.width || pos.y >= Place2D::view_rect.height)
		{
			return;
		}
		if(pos.x < 0 || pos.y < 0)
		{
			return;
		}
		MyAssert(pos.x < MAP_WIDTH && pos.y < MAP_HEIGHT);
		_map[pos.x][pos.y].is_exist = true;
	}

	void CalculateWithoutVisability()
	{
		for (int x = Place2D::view_rect.Width(); x >=0; x--)
		{
			for (int y = Place2D::view_rect.Height(); y >=0; y--)
			{
				MyAssert(x < MAP_WIDTH && y < MAP_HEIGHT);
				CellInfo &cell = _map[x][y];
				cell.pos =  IPoint(field_offset.x + x +1, field_offset.y + y+1)*GameSettings::SQUARE_SIDE;

				cell.exist_count = 0;
				for(size_t r = 0; r < 4; r++)
				{
					const IPoint &p_dir = dirs_ccw[r] + IPoint(x, y);

					MyAssert(p_dir.x < MAP_HEIGHT && p_dir.y < MAP_WIDTH);
					MyAssert(p_dir.x >= 0  && p_dir.y >= 0);

					CellInfo &cell_dir = _map[p_dir.x][p_dir.y];
					if(cell_dir.is_exist){
						cell.exist_count++;
					}
					for(size_t k = 0; k < 4;k++)
					{
						BYTE dir = (r + 4 - k) % 4;
						cell.coners[k][dir] = cell_dir.is_exist;
					}
				}
			}
		}
	}

	void DrawPlaceWithoutVisability(const Color &color, const IRect &rectDraw, float zDepth, const std::string &name_border)
	{
		if(!name_border.empty())
		{
			Place2D::mask_map = &Place2D::borders[name_border];
		}else{
			Place2D::mask_map = NULL;
		}
		for (int x = 0; x < Place2D::view_rect.Width(); x++)
		{
			for (int y = 0 ; y < Place2D::view_rect.Height(); y++)
			{
				MyAssert(x < MAP_WIDTH && y < MAP_HEIGHT);
				CellInfo &cell = _map[x][y];
				if(cell.exist_count == 0)
				{
					continue;
				}
				IRect rect_full(int(cell.pos.x), int(cell.pos.y), GameSettings::SQUARE_SIDE, GameSettings::SQUARE_SIDE);
				FRect frect_full = rect_full / rectDraw;
				Byte mask = 0;
				Byte one_bit = 1;
				for(size_t k = 0; k < 4; k++)
				{
					if(cell.coners[0][k])
					{
						mask |= one_bit;
					}
					one_bit = one_bit << 1;
				}
				if(mask != 0)
				{
					AddBorder(cell.pos, mask, 0.f);
				}
			}
		}
	}

	void BindBorders(std::vector<ScopePieceForDraw> *vec)
	{
		current_borders = vec;
	}
}
