#include "stdafx.h"
#include "CardResourceManager.h"


namespace  Card
{
	CardResourceManager cardResourceManager;
			
	/*******************/
	// CardResourceManager
	/*******************/

	const float CardResourceManager::LOAD_MARGIN = 3.0f;

	CardResourceManager::CardResource::CardResource(Resource *r, float low, float high)
		: res(r)
		, low(low)
		, high(high)
		, priority(0.0f)
		, used(false)
	{
	}

	CardResourceManager::~CardResourceManager()
	{
	}

	void CardResourceManager::Release()
	{
		for(Resources::iterator itr = _resources.begin(); itr != _resources.end(); ++itr)
		{
			if( itr->used ) {
				itr->res->EndUse(ResourceLoadMode::Async);
				itr->used = false;
			}
		}
	}

	void CardResourceManager::Register(Resource *res, const float low_border, const float high_border)
	{
		_resources.push_back( CardResource(res, low_border, high_border) );
	}

	bool CardResourceManager::Compare(const CardResourceManager::CardResource& r1, const CardResourceManager::CardResource &r2)
	{
		return r1.priority < r2.priority;
	}

	void CardResourceManager::Update(float low_level, float high_level, float scroll_speed)
	{
		_low = low_level;
		_high = high_level;
		float load_area_size = (_high - _low) * LOAD_MARGIN;
		float load_area_center = (_low + _high) * 0.5f;

		for(Resources::iterator itr = _resources.begin(); itr != _resources.end(); ++itr)
		{
			float distance;
			if( load_area_center > itr->low && load_area_center < itr->high)
				distance = 0.0f;
			else
				distance = std::min( math::abs(itr->low - load_area_center), math::abs(itr->high - load_area_center) );
			itr->priority = load_area_size * 0.5f - distance;
		}

		std::sort(_resources.begin(), _resources.end(), Compare);

		for(Resources::iterator itr = _resources.begin(); itr != _resources.end(); ++itr)
		{
			if(itr->priority < 0.0f)
			{
				// release
				if(itr->used && itr->res->IsLoaded()) {
					itr->res->EndUse(ResourceLoadMode::Async);
					itr->used = false;
				}
			}
			else
			{
				// upload
				if(!itr->used) {
					itr->res->BeginUse(ResourceLoadMode::Async);
					itr->used = true;
				}
			}
		}
	}

	void CardResourceManager::WaitForLoading()
	{
		for(Resources::iterator itr = _resources.begin(); itr != _resources.end(); ++itr)
		{
			if( itr->used )
			{
				itr->res->EnsureLoaded();
			}
		}
	}

	bool CardResourceManager::IsLoadedArea(float low, float high) const
	{
		float load_area_size = (high - low);
		float load_area_center = (low + high) * 0.5f;
		
		for(Resources::const_iterator itr = _resources.begin(); itr != _resources.end(); ++itr)
		{
			float distance;
			if( load_area_center > itr->low && load_area_center < itr->high)
				distance = 0.0f;
			else
				distance = std::min( math::abs(itr->low - load_area_center), math::abs(itr->high - load_area_center) );
			float priority = load_area_size * 0.5f - distance;
			
			if( priority >= 0.0f && !itr->res->IsLoaded() ) {
				return false;
			}
		}
		return true;
	}

	bool CardResourceManager::IsLoadedAll() const
	{
		for(Resources::const_iterator itr = _resources.begin(); itr != _resources.end(); ++itr)
		{
			if( itr->used && !itr->res->IsLoaded() )
				return false;
		}
		return true;
	}

}  // namespace Card
 