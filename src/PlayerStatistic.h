#ifndef _PLAYER_STATISTIC_H_
#define _PLAYER_STATISTIC_H_

#pragma once

class PlayerStatistic
{
	std::string _filename;
	std::vector< std::pair<std::string, std::string> > _data;
public:
	void StartCollect(const std::string &filename, const std::string &columns);
	void EndCollect();

	void SetValue(const std::string &column, const std::string &value);
	std::string GetValue(const std::string &column) const;
};

extern PlayerStatistic playerStatistic;

#endif