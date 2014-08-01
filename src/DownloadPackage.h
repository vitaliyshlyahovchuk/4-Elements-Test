#include <string>
#include <map>
#include <list>
#include <boost/function.hpp>

typedef boost::function<void()> UpdaterStartCallback;
typedef boost::function<void(bool)> UpdaterFinishCallback;

#ifdef __OBJC__

@interface DownloadPackage : NSObject
{
	UpdaterStartCallback cbStart;
	UpdaterFinishCallback cbFinish;
}
+(NSString *) md5:(NSData *) input;
-(void) downloadThreadOnce; // один раз проверяет апдейты и завершается
-(void) setStartCallback:(UpdaterStartCallback) cb;
-(void) setFinishCallback:(UpdaterFinishCallback) cb;

@end

#endif

//------------------------------------------------------------------------------

class PackageUpdater {
private:
	typedef std::map<std::string, std::string> FileHashes;
	typedef std::list<std::string> FileList;

	FileHashes mFileHashes;		//информация о текущей версии файлов
	FileList mFilesToDownload;	//файлы которые нужно обновить
	int currentBuild;
	bool needUpdate;
public:
	PackageUpdater();
	
	// выкачивает список файлов для обновления
    bool LoadNewFileList();
	
	// нужно ли чего обновлять?
    bool NeedUpdate();
	
	// непосредственно выкачивает файлы для обновления
    bool DownloadFiles();
    
	// загружает текущий (локальный) список файлов
	void LoadCurrentFileList();
    
	// заменяет папку с текущим апдейтом на свежевыкаченную папку
	bool SetNewFileList();
	
	// обрабатывает скаченный список файлов для обновления
	void CreateFileListsToUpdate();

	// скачивает один файл и подсчитывает его хеш
	bool DownloadFile(const std::string file, std::string &hash);
	
	// проверка, нормально ли скачался список файлов для обновления
	bool ValidateFilesList(const std::string file);

	// копируем файл из текущей папки в папку для закачек
    void CopyToTemp(const std::string file);
	
	// проверяет, соответствуют ли файлы в кеше версии приложения и при необходимости чистит кеш
	void CheckCache();
	
	// очищает временную папку, в коорую непосредственно происходит скачивание
	void ClearTemp();
	
	static std::string BACKUP_PATH; // бекап предыдущей версии файлов
	static std::string WORK_PATH;   // папка, примонтированная и используемая игрой
	static std::string TEMP_PATH;   // папка, куда закачиваются новые файлы
};

