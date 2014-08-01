#ifndef _DATA_STORE_WITH_RAPID_
#define _DATA_STORE_WITH_RAPID_

namespace Xml {

class TiXmlElement;

}

class DataVariableWithRapid
{
private:

	enum VARIABLE_TYPE { TYPE_NONE=0, TYPE_INT, TYPE_FLOAT, TYPE_DOUBLE, TYPE_STRING, TYPE_BOOLEAN, TYPE_POINT, TYPE_RECT, TYPE_ARRAY };

	VARIABLE_TYPE type;

	// simple types
	union {
		int iValue;
		float fValue;
		double dValue;
		bool bValue;
	};
	std::string sValue;
	IPoint iPValue;
	IRect iRValue;
	std::vector<DataVariableWithRapid> array;

public:
	DataVariableWithRapid();

	void Load(Xml::TiXmlElement* elem);
	void Save(Xml::TiXmlElement* elem);

	void Load(rapidxml::xml_node<>* elem);
	void Save(rapidxml::xml_node<>* elem);
	
	void Load(const std::string &strType, const std::string &strValue);
	void Save(std::string &strType, std::string &strValue);

	bool getBool() const;
	int getInt() const;
	float getFloat() const;
	double getDouble() const;
	std::string getString() const;
	IPoint getPoint() const;
	IRect getRect() const;

	void setArray();
	DataVariableWithRapid& getElement(int index);
	DataVariableWithRapid& appendElement();
	int arraySize() const;

	void setBool(bool value);
	void setInt(int value);
	void setFloat(float value);
	void setDouble(double value);
	void setString(const std::string &value);
	void setPoint(const IPoint &value);
	void setRect(const IRect &value);

	void setValue(const std::string &value); // задает значение в виде строки, не меняя при этом тип переменной
};

class DataStoreWithRapid
{
private:
	typedef std::map<std::string, DataVariableWithRapid> Data;

	Data _data;

	std::string DotToComma(const std::string &str);
	std::string CommaToDot(const std::string &str);

public:
	void Clear();

	/// Загружает данные, предварительно очищая существующие
	void Load(Xml::TiXmlElement* elem);
	void Load(rapidxml::xml_node<>* elem);
	/// Загружает данные, не очищая существующие (с перезаписью данных)
	void LoadAppend(Xml::TiXmlElement* elem);
	void LoadAppend(rapidxml::xml_node<>* elem);
	/// Сохраняет данные в xml
	void Save(Xml::TiXmlElement* elem);
	void Save(rapidxml::xml_node<>* elem);

	/// перезаписать значения
	void UpdateValues(const std::map<std::string, std::string> &data);

	void LoadCSV(const std::string &filename);
	void SaveCSV(const std::string &filename);

	bool findName(const std::string &name);
	void erase(const std::string &name);

	bool getBool(const std::string &varName, bool def = false);
	int getInt(const std::string &varName, int def = 0);
	float getFloat(const std::string &varName, float def = 0.0f);
	double getDouble(const std::string &varName, double def = 0.0f);
	std::string getString(const std::string &varName, const std::string &def = "");
	IPoint getPoint(const std::string &varName, const IPoint &def = IPoint());
	IRect getRect(const std::string &varName, const IRect &def = IRect());

	bool getArrBool(const std::string &varName, int index, bool def = false);
	int getArrInt(const std::string &varName, int index, int def = 0);
	float getArrFloat(const std::string &varName, int index, float def = 0.0f);
	double getArrDouble(const std::string &varName, int index, double def = 0.0f);
	std::string getArrString(const std::string &varName, int index, const std::string &def = "");
	IPoint getArrPoint(const std::string &varName, int index, const IPoint &def = IPoint());
	IRect getArrRect(const std::string &varName, int index, const IRect &def = IRect());
	int getArraySize(const std::string &varName);
	
	void setBool(const std::string &varName, bool value);
	void setInt(const std::string &varName, int value);
	void setFloat(const std::string &varName, float value);
	void setDouble(const std::string &varName, double value);
	void setString(const std::string &varName, const std::string &value);
	void setPoint(const std::string &varName, const IPoint &value);
	void setRect(const std::string &varName, const IRect &value);

	void setArrBool(const std::string &varName, int index, bool value);
	void setArrInt(const std::string &varName, int index, int value);
	void setArrFloat(const std::string &varName, int index, float value);
	void setArrDouble(const std::string &varName, int index, double value);
	void setArrString(const std::string &varName, int index, const std::string &value);
	void setArrPoint(const std::string &varName, int index, const IPoint &value);
	void setArrRect(const std::string &varName, int index, const IRect &value);
};


	
#endif //_DATA_STORE_WITH_RAPID_