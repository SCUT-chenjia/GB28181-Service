﻿#pragma once
#include "Structs.h"

class XmlParser
{
public:
	bool Parse(const char* data,int len, pugi::xml_document& doc);
	bool ParseHeader(manscdp_msgbody_header_t& header, pugi::xml_document& doc);
private:
};

