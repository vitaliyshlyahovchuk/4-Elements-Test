#include "stdafx.h"
#include "PlayerStatistic.h"
#include <boost/algorithm/string.hpp>

PlayerStatistic playerStatistic;

void PlayerStatistic::StartCollect(const std::string &filename, const std::string &columns)
{
	_filename = filename;

	_data.clear();

	std::vector<std::string> columnNames;
	boost::split(columnNames, columns, boost::is_any_of(",;"));

	for(size_t i = 0; i < columnNames.size(); ++i )
	{
		_data.push_back( std::make_pair(columnNames[i], "") );
	}
}

void PlayerStatistic::EndCollect()
{
	IO::OutputStreamPtr file;
	std::string buffer;
	if( Core::fileSystem.FileExists(_filename) )
	{
		file = Core::fileSystem.OpenUpdate(_filename);
		file->Seek(0, IO::Origin::End);
	}
	else
	{
		file = Core::fileSystem.OpenWrite(_filename);

		// записываем заголовки столбцов
		for( size_t i = 0; i < _data.size(); ++i ){
			if( i > 0 )
				buffer.push_back(',');
			buffer.append(_data[i].first);
		}
		file->Write(buffer.c_str(), buffer.size());
	}
	
	buffer.clear();
	buffer.push_back('\n');
	// записываем данные
	for( size_t i = 0; i < _data.size(); ++i ){
		if( i > 0 )
			buffer.push_back(',');
		buffer.append(_data[i].second);
	}
	file->Write(buffer.c_str(), buffer.size());
	file->Flush();
}

void PlayerStatistic::SetValue(const std::string &column, const std::string &value)
{
	for(size_t i = 0; i < _data.size(); ++i){
		if(_data[i].first == column ) {
			_data[i].second = value;
			break;
		}
	}
}

std::string PlayerStatistic::GetValue(const std::string &column) const
{
	for(size_t i = 0; i < _data.size(); ++i){
		if(_data[i].first == column )
			return _data[i].second;
	}
	return "";
}