#include "stdafx.h"
#include "types.h"
#include "DataStoreWithRapid.h"
#include "EngineAssert.h"

DataVariableWithRapid::DataVariableWithRapid()
	: type(TYPE_NONE)
{
}

bool DataVariableWithRapid::getBool() const
{
	Assert(type == TYPE_BOOLEAN);
	return bValue;
}

int DataVariableWithRapid::getInt() const
{
	Assert(type == TYPE_INT);
	return iValue;
}

float DataVariableWithRapid::getFloat() const
{
	Assert(type == TYPE_FLOAT);
	return fValue;
}

double DataVariableWithRapid::getDouble() const
{
	Assert(type == TYPE_DOUBLE);
	return dValue;
}

std::string DataVariableWithRapid::getString() const
{
    switch (type) {
        case TYPE_ARRAY:
        case TYPE_POINT:
        case TYPE_RECT:
        case TYPE_NONE:
        default:
            Assert(false);
        case TYPE_BOOLEAN:
            return utils::lexical_cast(bValue);
            break;
        case TYPE_FLOAT:
            return utils::lexical_cast(fValue);
            break;
        case TYPE_DOUBLE:
            return utils::lexical_cast(dValue);
            break;
        case TYPE_INT:
            return utils::lexical_cast(iValue);
            break;
        case TYPE_STRING:
            return sValue;
            break;
    }

}

IPoint DataVariableWithRapid::getPoint() const
{
	Assert(type == TYPE_POINT);
	return iPValue;
}

IRect DataVariableWithRapid::getRect() const
{
	Assert(type == TYPE_RECT);
	return iRValue;
}

void DataVariableWithRapid::setArray()
{
	type = TYPE_ARRAY;
}

DataVariableWithRapid& DataVariableWithRapid::getElement(int index)
{
	Assert(type == TYPE_ARRAY);
	Assert(index >= 0);
	Assert(index < (int)array.size());
	return array[index];
}

DataVariableWithRapid& DataVariableWithRapid::appendElement()
{
	type = TYPE_ARRAY;
	array.push_back(DataVariableWithRapid());
	return array.back();
}

int DataVariableWithRapid::arraySize() const
{
	Assert(type == TYPE_ARRAY);
	return (int)array.size();
}

void DataVariableWithRapid::setBool(bool value)
{
	type = TYPE_BOOLEAN;
	bValue = value;
}

void DataVariableWithRapid::setInt(int value)
{
	type = TYPE_INT;
	iValue = value;
}

void DataVariableWithRapid::setFloat(float value)
{
	type = TYPE_FLOAT;
	fValue = value;
}

void DataVariableWithRapid::setDouble(double value)
{
	type = TYPE_DOUBLE;
	dValue = value;
}

void DataVariableWithRapid::setString(const std::string &value)
{
	type = TYPE_STRING;
	sValue = value;
}

void DataVariableWithRapid::setPoint(const IPoint &value)
{
	type = TYPE_POINT;
	iPValue = value;
}

void DataVariableWithRapid::setRect(const IRect &value)
{
	type = TYPE_RECT;
	iRValue = value;
}

void DataVariableWithRapid::Load(Xml::TiXmlElement* elem)
{
	if (!elem->Attribute("type")) {
		type = TYPE_NONE;
		return;
	}

	const char * strType = elem->Attribute("type");
	Assert(strType != NULL);
	if (utils::equals(strType, "array")) {
		type = TYPE_ARRAY;
		Xml::TiXmlElement* el = elem->FirstChildElement("Data");
		while(el) {
			appendElement().Load(el);
			el = el->NextSiblingElement("Data");
		}
		return;
	}
	const char *strValue = elem->Attribute("value");
	if (utils::equals(strType, "int")) {
		type = TYPE_INT;
		iValue = 0;
		if (strValue) {
			iValue = utils::lexical_cast<int>(strValue);
		}
	} else if (utils::equals(strType, "float")) {
		type = TYPE_FLOAT;
		fValue = 0;
		if (strValue) {
			fValue = utils::lexical_cast<float>(strValue);
		}
	} else if (utils::equals(strType, "double")) {
		type = TYPE_DOUBLE;
		dValue = 0;
		if (strValue) {
			dValue = utils::lexical_cast<double>(strValue);
		}
	} else if (utils::equals(strType, "bool")) {
		type = TYPE_BOOLEAN;
		bValue = false;
		if (strValue) {
			bValue = utils::lexical_cast<bool>(strValue);
		}
	} else if (utils::equals(strType, "string")) {
		type = TYPE_STRING;
		sValue = "";
		if (strValue) {
			sValue = strValue;
		}
	} else if (utils::equals(strType, "point")) {
		type = TYPE_POINT;
		iPValue = IPoint();
		const char *xValue = elem->Attribute("x"), *yValue = elem->Attribute("y");
		if (xValue) {
			iPValue.x = utils::lexical_cast<int>(xValue);
		}
		if (yValue) {
			iPValue.y = utils::lexical_cast<int>(yValue);
		}
	} else if (utils::equals(strType, "rect")) {
		type = TYPE_RECT;
		iRValue = IRect(elem);
	} else {
		type = TYPE_NONE;
	}
}

void DataVariableWithRapid::Save(Xml::TiXmlElement* elem)
{
	if (type == TYPE_NONE) {
		elem->SetAttribute("type", "none");
	} else if (type == TYPE_INT) {
		elem->SetAttribute("type", "int");
		elem->SetAttribute("value", utils::lexical_cast(iValue));
	} else if (type == TYPE_FLOAT) {
		elem->SetAttribute("type", "float");
		elem->SetAttribute("value", utils::lexical_cast(fValue));
	} else if (type == TYPE_DOUBLE) {
		elem->SetAttribute("type", "double");
		elem->SetAttribute("value", utils::lexical_cast(dValue));
	} else if (type == TYPE_BOOLEAN) {
		elem->SetAttribute("type", "bool");
		elem->SetAttribute("value", utils::lexical_cast(bValue));
	} else if (type == TYPE_STRING) {
		elem->SetAttribute("type", "string");
		elem->SetAttribute("value", sValue);
	} else if (type == TYPE_POINT) {
		elem->SetAttribute("type", "point");
		elem->SetAttribute("x", utils::lexical_cast(iPValue.x));
		elem->SetAttribute("y", utils::lexical_cast(iPValue.y));
	} else if (type == TYPE_RECT) {
		elem->SetAttribute("type", "rect");
		elem->SetAttribute("x", utils::lexical_cast(iRValue.x));
		elem->SetAttribute("y", utils::lexical_cast(iRValue.y));
		elem->SetAttribute("width", utils::lexical_cast(iRValue.width));
		elem->SetAttribute("height", utils::lexical_cast(iRValue.height));
	} else if (type == TYPE_ARRAY) {
		elem->SetAttribute("type", "array");
		for(size_t i = 0; i < array.size(); i++) {
			Xml::TiXmlElement* dataElem = elem->InsertEndChild(Xml::TiXmlElement("Data"))->ToElement();
			array[i].Save(dataElem);
		}
	}
}

void DataVariableWithRapid::Save(rapidxml::xml_node<>* elem)
{
	if (type == TYPE_NONE) {
		Xml::SetStringAttribute(elem, "type", "none");
	} else if (type == TYPE_INT) {
		Xml::SetStringAttribute(elem, "type", "int");
		Xml::SetIntAttribute(elem, "value", iValue);
	} else if (type == TYPE_FLOAT) {
		Xml::SetStringAttribute(elem, "type", "float");
		Xml::SetFloatAttribute(elem, "value", fValue);
	} else if (type == TYPE_DOUBLE) {
		Xml::SetStringAttribute(elem, "type", "double");
		Xml::SetStringAttribute(elem, "value", utils::lexical_cast(dValue));
	} else if (type == TYPE_BOOLEAN) {
		Xml::SetStringAttribute(elem, "type", "bool");
		Xml::SetBoolAttribute(elem, "value", bValue);
	} else if (type == TYPE_STRING) {
		Xml::SetStringAttribute(elem, "type", "string");
		Xml::SetStringAttribute(elem, "value", sValue);
	} else if (type == TYPE_POINT) {
		Xml::SetStringAttribute(elem, "type", "point");
		Xml::SetIntAttribute(elem, "x", iPValue.x);
		Xml::SetIntAttribute(elem, "y", iPValue.y);
	} else if (type == TYPE_RECT) {
		Xml::SetStringAttribute(elem, "type", "rect");
		Xml::SetIntAttribute(elem, "x", iRValue.x);
		Xml::SetIntAttribute(elem, "y", iRValue.y);
		Xml::SetIntAttribute(elem, "width", iRValue.width);
		Xml::SetIntAttribute(elem, "height", iRValue.height);
	} else if (type == TYPE_ARRAY) {
		Xml::SetStringAttribute(elem, "type", "array");
		for(size_t i = 0; i < array.size(); i++) {
			rapidxml::xml_node<>* dataElem = Xml::NewNode(elem, "Data");
			array[i].Save(dataElem);
		}
	}
}

void DataVariableWithRapid::Load(const std::string &strType, const std::string &strValue)
{
	if (!strType.size()) {
		type = TYPE_NONE;
		return;
	}

	if (strType == "int") {
		type = TYPE_INT;
		iValue = 0;
		iValue = utils::lexical_cast<int>(strValue);
	} else if (strType == "float") {
		type = TYPE_FLOAT;
		fValue = 0;
		fValue = utils::lexical_cast<float>(strValue);
	} else if (strType == "double") {
		type = TYPE_DOUBLE;
		dValue = 0;
		dValue = utils::lexical_cast<double>(strValue);
	} else if (strType == "bool") {
		type = TYPE_BOOLEAN;
		bValue = false;
		bValue = utils::lexical_cast<bool>(strValue);
	} else if (strType == "string") {
		type = TYPE_STRING;
		//sValue = std::string(""); в верху, присвоение сдалано так, чтобы Union обнулялся правильно, для строки такое не нужно, т.к. она хранится в другом месте
		sValue = strValue;
	} else if (strType == "array") {
		Assert("can't load an array from the std::string" && false);
	} else  {
		type = TYPE_NONE;
	}
}

void DataVariableWithRapid::Load(rapidxml::xml_node<>* elem)
{
	if (!Xml::HasAttribute(elem, "type")) {
		type = TYPE_NONE;
		return;
	}

	std::string strType = Xml::GetStringAttribute(elem, "type");
	if (strType == "array") {
		type = TYPE_ARRAY;
		rapidxml::xml_node<>* el = elem->first_node("Data");
		while(el) {
			appendElement().Load(el);
			el = el->next_sibling("Data");
		}
		return;
	}
	//const char *strValue = elem->Attribute("value");
	if (strType == "int") {
		type = TYPE_INT;
		iValue = 0;
		iValue = Xml::GetIntAttribute(elem, "value");
	} else if (strType == "float") {
		type = TYPE_FLOAT;
		fValue = 0;
		fValue = Xml::GetFloatAttribute(elem, "value");
	} else if (strType == "double") {
		type = TYPE_DOUBLE;
		dValue = 0;
		if(elem->first_attribute("value")) {
			dValue = utils::lexical_cast<double>(elem->first_attribute("value")->value());
		}
	} else if (strType ==  "bool") {
		type = TYPE_BOOLEAN;
		bValue = false;
		bValue = Xml::GetBoolAttribute(elem, "value");
	} else if (strType ==  "string") {
		type = TYPE_STRING;
		sValue = "";
		sValue = Xml::GetStringAttribute(elem, "value");
	} else if (strType ==  "point") {
		type = TYPE_POINT;
		iPValue = IPoint(elem);
	} else if (strType ==  "rect") {
		type = TYPE_RECT;
		iRValue = IRect(elem);
	} else {
		type = TYPE_NONE;
	}
}
void DataVariableWithRapid::Save(std::string &strType, std::string &strValue)
{
	if (type == TYPE_NONE) {
		strType = "none";
		strValue = "";
	} else if (type == TYPE_INT) {
		strType = "int";
		strValue = utils::lexical_cast(iValue);
	} else if (type == TYPE_FLOAT) {
		strType = "float";
		strValue = utils::lexical_cast(fValue);
	} else if (type == TYPE_DOUBLE) {
		strType = "double";
		strValue = utils::lexical_cast(dValue);
	} else if (type == TYPE_BOOLEAN) {
		strType = "bool";
		strValue = utils::lexical_cast(bValue);
	} else if (type == TYPE_STRING) {
		strType = "string";
		strValue = sValue;
	} else if(type == TYPE_ARRAY) {
		Assert("unable to save array to the std::string" && false);
	}
}

void DataVariableWithRapid::setValue(const std::string& value)
{
	switch( type )
	{
		case TYPE_BOOLEAN:
			bValue = (value != "0") && (value != "false");
			break;
		case TYPE_FLOAT:
			fValue = utils::lexical_cast<float>(value);
			break;
		case TYPE_DOUBLE:
			dValue = utils::lexical_cast<double>(value);
			break;
		case TYPE_INT:
			iValue = utils::lexical_cast<int>(value);
			break;
		case TYPE_STRING:
			sValue = value;
		default:
			Assert(false);
	}
}
///////////////////////////////////////////////////////////////////////////////////

void DataStoreWithRapid::Clear()
{
	_data.clear();
}

void DataStoreWithRapid::Load(Xml::TiXmlElement* elem)
{
	Clear();
	LoadAppend(elem);
}
void DataStoreWithRapid::Load(rapidxml::xml_node<>* elem)
{
	Clear();
	LoadAppend(elem);
}

void DataStoreWithRapid::LoadAppend(Xml::TiXmlElement* elem)
{
	Xml::TiXmlElement *dataElem = elem->FirstChildElement("Data");
	while (dataElem) {
		std::string name = dataElem->Attribute("name");
		_data[name].Load(dataElem);
		dataElem = dataElem->NextSiblingElement("Data");
	}
}

void DataStoreWithRapid::LoadAppend(rapidxml::xml_node<>* elem)
{
	rapidxml::xml_node<> *dataElem = elem->first_node("Data");
	while (dataElem) {
		std::string name = Xml::GetStringAttribute(dataElem, "name");
		_data[name].Load(dataElem);
		dataElem = dataElem->next_sibling("Data");
	}
}

void DataStoreWithRapid::Save(Xml::TiXmlElement* elem)
{
	for (Data::iterator it = _data.begin(); it != _data.end(); it++) {
		Xml::TiXmlElement* dataElem = elem->InsertEndChild(Xml::TiXmlElement("Data"))->ToElement();
		dataElem->SetAttribute("name", it->first.c_str());
		it->second.Save(dataElem);
	}
}
void DataStoreWithRapid::Save(rapidxml::xml_node<>* elem)
{
	for (Data::iterator it = _data.begin(); it != _data.end(); it++) {
		rapidxml::xml_node<>* dataElem = Xml::NewNode(elem, "Data");
		Xml::SetStringAttribute(dataElem, "name", it->first.c_str());
		it->second.Save(dataElem);
	}
}

void DataStoreWithRapid::UpdateValues(const std::map<std::string, std::string>& data)
{
	for(std::map<std::string, std::string>::const_iterator itr = data.begin(); itr != data.end(); ++itr)
	{
		Data::iterator i = _data.find(itr->first);
		if( i == _data.end() ){
			setString(itr->first, itr->second);
		} else {
			i->second.setValue(itr->second);
		}
	}
}

bool DataStoreWithRapid::findName(const std::string &name)
{
	Data::iterator it = _data.find(name);
	return (it != _data.end());
}

void DataStoreWithRapid::erase(const std::string &name)
{
	Data::iterator it = _data.find(name);
	if (it != _data.end()) {
		_data.erase(it);
	}
}

bool DataStoreWithRapid::getBool(const std::string &varName, bool def)
{
	if (findName(varName)) {
		return _data[varName].getBool();
	}
	return def;
}

int DataStoreWithRapid::getInt(const std::string &varName, int def)
{
	if (findName(varName)) {
		return _data[varName].getInt();
	}
	return def;
}

float DataStoreWithRapid::getFloat(const std::string &varName, float def)
{
	if (findName(varName)) {
		return _data[varName].getFloat();
	}
	return def;
}

double DataStoreWithRapid::getDouble(const std::string &varName, double def)
{
	if (findName(varName)) {
		return _data[varName].getDouble();
	}
	return def;
}

std::string DataStoreWithRapid::getString(const std::string &varName, const std::string &def)
{
	if (findName(varName)) {
		return _data[varName].getString();
	}
	return def;
}

IPoint DataStoreWithRapid::getPoint(const std::string &varName, const IPoint &def)
{
	if (findName(varName)) {
		return _data[varName].getPoint();
	}
	return def;
}

IRect DataStoreWithRapid::getRect(const std::string &varName, const IRect &def)
{
	if (findName(varName)) {
		return _data[varName].getRect();
	}
	return def;
}

bool DataStoreWithRapid::getArrBool(const std::string &varName, int index, bool def)
{
	if (findName(varName) && _data[varName].arraySize() > index) {
		return _data[varName].getElement(index).getBool();
	}
	return def;
}

int DataStoreWithRapid::getArrInt(const std::string &varName, int index, int def)
{
	if (findName(varName) && _data[varName].arraySize() > index) {
		return _data[varName].getElement(index).getInt();
	}
	return def;
}

float DataStoreWithRapid::getArrFloat(const std::string &varName, int index, float def)
{
	if (findName(varName) && _data[varName].arraySize() > index) {
		return _data[varName].getElement(index).getFloat();
	}
	return def;
}

double DataStoreWithRapid::getArrDouble(const std::string &varName, int index, double def)
{
	if (findName(varName) && _data[varName].arraySize() > index) {
		return _data[varName].getElement(index).getDouble();
	}
	return def;
}

std::string DataStoreWithRapid::getArrString(const std::string &varName, int index, const std::string &def)
{
	if(findName(varName) && _data[varName].arraySize() > index) {
		return _data[varName].getElement(index).getString();
	}
	return def;
}

IPoint DataStoreWithRapid::getArrPoint(const std::string &varName, int index, const IPoint &def)
{
	if(findName(varName) && _data[varName].arraySize() > index) {
		return _data[varName].getElement(index).getPoint();
	}
	return def;
}

IRect DataStoreWithRapid::getArrRect(const std::string &varName, int index, const IRect &def)
{
	if(findName(varName) && _data[varName].arraySize() > index) {
		return _data[varName].getElement(index).getRect();
	}
	return def;
}

int DataStoreWithRapid::getArraySize(const std::string &varName)
{
	if(findName(varName)) {
		return _data[varName].arraySize();
	}
	return -1;
}

void DataStoreWithRapid::setBool(const std::string &varName, bool value)
{
	_data[varName].setBool(value);
}

void DataStoreWithRapid::setInt(const std::string &varName, int value)
{
	_data[varName].setInt(value);
}

void DataStoreWithRapid::setFloat(const std::string &varName, float value)
{
	_data[varName].setFloat(value);
}

void DataStoreWithRapid::setDouble(const std::string &varName, double value)
{
	_data[varName].setDouble(value);
}

void DataStoreWithRapid::setString(const std::string &varName, const std::string &value)
{
	_data[varName].setString(value);
}

void DataStoreWithRapid::setPoint(const std::string &varName, const IPoint &value)
{
	_data[varName].setPoint(value);
}

void DataStoreWithRapid::setRect(const std::string &varName, const IRect &value)
{
	_data[varName].setRect(value);
}

void DataStoreWithRapid::setArrBool(const std::string &varName, int index, bool value)
{
	if (!findName(varName)) {
		_data[varName].setArray();
	}
	DataVariableWithRapid &data = _data[varName];
	while (data.arraySize() <= index) {
		data.appendElement().setBool(false);
	}
	data.getElement(index).setBool(value);
}

void DataStoreWithRapid::setArrInt(const std::string &varName, int index, int value)
{
	if (!findName(varName)) {
		_data[varName].setArray();
	}
	DataVariableWithRapid &data = _data[varName];
	while (data.arraySize() <= index) {
		data.appendElement().setInt(0);
	}
	data.getElement(index).setInt(value);
}

void DataStoreWithRapid::setArrFloat(const std::string &varName, int index, float value)
{
	if (!findName(varName)) {
		_data[varName].setArray();
	}
	DataVariableWithRapid &data = _data[varName];
	while (data.arraySize() <= index) {
		data.appendElement().setFloat(0.f);
	}
	data.getElement(index).setFloat(value);
}

void DataStoreWithRapid::setArrDouble(const std::string &varName, int index, double value)
{
	if (!findName(varName)) {
		_data[varName].setArray();
	}
	DataVariableWithRapid &data = _data[varName];
	while (data.arraySize() <= index) {
		data.appendElement().setDouble(0.0);
	}
	data.getElement(index).setDouble(value);
}

void DataStoreWithRapid::setArrString(const std::string &varName, int index, const std::string &value)
{
	if (!findName(varName)) {
		_data[varName].setArray();
	}
	DataVariableWithRapid &data = _data[varName];
	while (data.arraySize() <= index) {
		data.appendElement().setString("");
	}
	data.getElement(index).setString(value);
}

void DataStoreWithRapid::setArrPoint(const std::string &varName, int index, const IPoint &value)
{
	if (!findName(varName)) {
		_data[varName].setArray();
	}
	DataVariableWithRapid &data = _data[varName];
	while (data.arraySize() <= index) {
		data.appendElement().setPoint(IPoint());
	}
	data.getElement(index).setPoint(value);
}

void DataStoreWithRapid::setArrRect(const std::string &varName, int index, const IRect &value)
{
	if (!findName(varName)) {
		_data[varName].setArray();
	}
	DataVariableWithRapid &data = _data[varName];
	while (data.arraySize() <= index) {
		data.appendElement().setRect(IRect());
	}
	data.getElement(index).setRect(value);
}

std::string DataStoreWithRapid::DotToComma(const std::string &str)
{
	std::string retval(str);
	for (size_t i = 0 ; i < str.length() ; ++i) {
		if (retval[i] == '.') {
			retval[i] = ',';
		}
	}
	return retval;
}

std::string DataStoreWithRapid::CommaToDot(const std::string &str)
{
	std::string retval(str);
	for (size_t i = 0 ; i < str.length() ; ++i) {
		if (retval[i] == ',') {
			retval[i] = '.';
		}
	}
	return retval;
}

void DataStoreWithRapid::LoadCSV(const std::string &filename)
{
#ifdef ENGINE_TARGET_WIN32
	_data.clear();
	FILE *file = utils::fopen(filename.c_str(), "r");
	if (!file) {
		return;
	}
	char linebuf[2048];
	while (fgets(linebuf, 2048, file))
	{
		std::string line(linebuf);
		std::string name = line.substr(0, line.find_first_of(";"));
		line = line.substr(line.find_first_of(";") + 1, line.length() - line.find_first_of(";") - 1);
		std::string type = line.substr(0, line.find_first_of(";"));
		line = line.substr(line.find_first_of(";") + 1, line.length() - line.find_first_of(";") - 1);
		std::string value = line.substr(0, line.length() - 1);
		DataVariableWithRapid data;
		data.Load(type, CommaToDot(value));
		_data.insert(std::make_pair(name, data));
	}
	fclose(file);
#else
	// not implemented
	Assert(false);
#endif
}

void DataStoreWithRapid::SaveCSV(const std::string &filename)
{
#ifdef ENGINE_TARGET_WIN32
	FILE *file = utils::fopen(filename.c_str(), "w");
	if (!file)
		return;
	std::string strType, strValue;
	for (std::map<std::string, DataVariableWithRapid>::iterator it = _data.begin(); it != _data.end(); it++)
	{
		it->second.Save(strType, strValue);
		fprintf(file, "%s", it->first.c_str());
		fprintf(file, ";");
		fprintf(file, "%s", strType.c_str());
		fprintf(file, ";");
		fprintf(file, "%s", DotToComma(strValue).c_str());
		fprintf(file, "\n");
	}
	fclose(file);
#else
	// not implemented
	Assert(false);
#endif
}
