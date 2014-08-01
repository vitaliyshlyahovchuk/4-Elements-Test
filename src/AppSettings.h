#pragma once

// внешние настройки приложения (в версиях dev,ent)

class AppSettings {
public:
	static void CheckReset();
	
	static bool getBool(const std::string name);
};