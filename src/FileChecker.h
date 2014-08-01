#pragma once

#include <map>
// класс для проверки hash суммы файлов (аналого xor-шифрования, только это у нас будет цифровой подписью)
// у каждого файла храним его hash
// при изменении файла читером, hash изменится, и мы поймем, что это читер :)
// hash = md5 (планирую еще добавить "соли" туда, но потом)
class FileChecker {
public:
	static void check();
private:
	typedef std::map<std::string, std::string> tHashMap;
	typedef tHashMap::iterator tHashMapIterator;
};