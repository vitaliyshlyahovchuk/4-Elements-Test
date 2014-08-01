#pragma once

// класс для проверки текущей версии.
// текущая версия  - bundle version
// загрузка номера возможно новой версии осуществляется с помощью DownloadPackage
// DownloadPackage загружает файл с версиями - versions.xml (с сервера)
// Пример файла versions.xml:
/*
<root>
	<item bundleVersion="1.0.0"/>
	<item bundleVersion="1.0.1" mandatory="false"/>
	<item bundleVersion="1.1.0" mandatory="true"/>
	<item bundleVersion="1.1.1" mandatory="false"/>
	<item bundleVersion="1.1.2" mandatory="false"/>
</root>
*/
// Если mandatory = true, то требуется принудительное обновление игры

class GameUpdateChecker {
public:
	static void check();
private:
	static void updateNowHandler();
	static void updateLaterHandler();
};