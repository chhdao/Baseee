/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 这个文件来自 GOSCPS(https://github.com/GOSCPS)
 * 使用 GOSCPS 许可证
 * File:    jsonp.cpp
 * Content: json parser Source File
 * Copyright (c) 2020 GOSCPS 保留所有权利.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <regex>
#include <array>
#include <optional>
#include <variant>
#include <map>
#include "jsonp.hpp"
#include "../baseee-test/test.hpp"
#include "../baseee-string/string.hpp"

//判断字符是否处于a-b范围内
//b >= a
inline bool baseee::parser::JsonParser::MatchCharRange(char a, char b) noexcept {
	if ((*Next) >= a && (*Next) <= b) return true;
	else return false;
}

baseee::parser::JsonErrCode 
baseee::parser::JsonParser::Parser(std::string_view JsonStr) noexcept {
	if (std::string(JsonStr).empty()) return JsonErrCode::Parse_OK;

	this->Json = std::string(JsonStr);
	this->Next = Json.cbegin();

	this->AfterSpace();
	auto Result = this->ParseValue();

	if (Result == JsonErrCode::Parse_OK) {
		AfterSpace();
		if (Next != Json.cend()) return JsonErrCode::Parse_ValueError;
	}

	JsonPool = JsonNext;
	return Result;
}

//跳过空格
void baseee::parser::JsonParser::AfterSpace() noexcept {
	if (Next == Json.cend()) return;
	while (std::isspace(*Next)) Next++;
	return;
}

//解析值
baseee::parser::JsonErrCode 
baseee::parser::JsonParser::ParseValue() noexcept {
	switch (*Next) {
	case 'n': return ParseValueNull();//'n'ull
	case '\0': return JsonErrCode::Parse_MissValue;

	case 'f': //'f'alse
	case 't'://'t'rue
		return ParseValueBool();

	case '\"': return ParseValueString();//'"' String"
	case '[': return ParseValueArray();//'['xxx,xxx]
	case '{': return ParseValueObject();//'{' "xxx":xxx}
	default: return ParseValueNumber();//(-?)|([0-9]?)Number
	}
}

//解析null
baseee::parser::JsonErrCode 
baseee::parser::JsonParser::ParseValueNull() noexcept {

	if (!IteratorMatch("null")) {
		return JsonErrCode::Parse_ValueError;
	}
	Next += 4;

	JsonNext.JsonT = JsonType::JsonType_Null;
	JsonNext.Data = 0.0;
	return JsonErrCode::Parse_OK;
}

//解析true
baseee::parser::JsonErrCode 
baseee::parser::JsonParser::ParseValueTrue() noexcept {

	if (!IteratorMatch("true")) {
		return JsonErrCode::Parse_ValueError;
	}
	Next += 4;

	JsonNext.JsonT = JsonType::JsonType_True;
	JsonNext.Data = 0.0;
	return JsonErrCode::Parse_OK;
}

//解析false
baseee::parser::JsonErrCode 
baseee::parser::JsonParser::ParseValueFalse() noexcept {

	if (!IteratorMatch("false")) {
		return JsonErrCode::Parse_ValueError;
	}
	Next += 5;

	JsonNext.JsonT = JsonType::JsonType_False;
	JsonNext.Data = 0.0;
	return JsonErrCode::Parse_OK;
}

//解析Bool
baseee::parser::JsonErrCode
baseee::parser::JsonParser::ParseValueBool() noexcept {

	if ((*Next) == 't') {
		return this->ParseValueTrue();
	}
	else if ((*Next) == 'f') {
		return this->ParseValueFalse();
	}
	else return JsonErrCode::Parse_ValueError;
}

//字符匹配
bool baseee::parser::JsonParser::IteratorMatch(std::string_view str) noexcept {
	std::string sstr(str);

	if (sstr[sstr.size() - 1] == '\0') sstr.erase(sstr[sstr.size() - 1]);

	for (int a = 0; a < sstr.size(); a++) {
		if ((Next + a) == Json.cend()) return false;
		else if ((*(Next+a)) != sstr[a]) return false;
		else continue;
	}
	return true;
}


//解析数字
baseee::parser::JsonErrCode 
baseee::parser::JsonParser::ParseValueNumber() noexcept {

	std::string Int("([-]?[1-9]?[0-9]+)|([-]?[0]{1})");
	std::string Frac("\\.[0-9]+");
	std::string Exp("[eE]{1}[+-]?[0-9]+");
	std::string Number("(" + Int + "){1}(" + Frac + ")?(" + Exp + ")?");

	std::regex NumberRegex(Number);

	char c[] = { *Next,'\0' };
	std::string NumberBuf(c);

	//循环读取数字
	//数字为 '0'~'9' '-' 'e' 'E' '.'中的任意一个
	while (Next != Json.cend() &&
		(MatchCharRange('0', '9') 
			|| (*Next) == '-'
			|| (*Next) == 'e' 
			|| (*Next) == 'E'
			|| (*Next) == '.')) {
		c[0] = { *Next };
		NumberBuf.append(std::string(c));
		Next++;
	}

	if (!std::regex_match(NumberBuf, NumberRegex)) {
		return JsonErrCode::Parse_ValueError;
	}

	this->JsonNext.JsonT = JsonType::JsonType_Number;
	this->JsonNext.Data = std::stod(NumberBuf);

	return JsonErrCode::Parse_OK;
}

baseee::parser::JsonErrCode 
baseee::parser::JsonParser::ParseValueString() noexcept {
	std::string String = "";
	Next++;

	std::optional<std::string> opt;

	char c[] = { *Next,'\0' };
	
	while (true) {

		if (Next == Json.cend()) return JsonErrCode::Parse_MissToken;
		else if ((*Next) == '\"') {
			Next++;
			break;
		}
		else if ((*Next) == '\\') {
			if ((Next++) == Json.cend()) return JsonErrCode::Parse_MissToken;
			else switch (*Next) {
			case 'n':
				String.append("\n");
				break;
			case '\"':
				String.append("\"");
				break;
			case '\\':
				String.append("\\");
				break;
			case 'b':
				String.append("\b");
				break;
			case 'f':
				String.append("\f");
				break;
			case 'r':
				String.append("\r");
				break;
			case 't':
				String.append("\t");
				break;
			case 'u':
				opt = ParseValueUnicode();
				if(opt == std::nullopt || !opt.has_value()) return JsonErrCode::Parse_ValueError;
				String.append(opt.value());
				break;
			default:
				return JsonErrCode::Parse_ValueError;
			}
		}
		else {
			c[0] = *Next;
			String.append(c);
		}

		Next++;
	}

	this->JsonNext.JsonT = JsonType::JsonType_String;
	this->JsonNext.Data = String;

	return JsonErrCode::Parse_OK;
}

//解析\\uXXXX
//以及\\uXXXX\\uYYYY(代理)
std::optional<std::string> 
baseee::parser::JsonParser::ParseValueUnicode() noexcept {
	Next++;//跳过\\u中的u

	uint32_t Out;

	std::string StrBuf = "0x";//解析16进制数字需要0x
	char c[] = { '\0','\0' };

	//读取数字
	for (int a = 0; a < 4; a++) {
		if (Next == Json.cend()) {
			return std::nullopt;
		}
		c[0] = *Next;
		StrBuf.append(c);
		Next++;
	}

	uint32_t U8 = std::stoul(StrBuf);

	//为高位代理
	//继续读取低位代理
	if (U8 >= 0xD800 && U8 <= 0xDBFF) {
		if (!IteratorMatch("\\u")) return std::nullopt;
		Next += 2;

		StrBuf = "0x";
		for (int a = 0; a < 4; a++) {
			if (Next == Json.cend()) {
				return std::nullopt;
			}
			c[0] = *Next;
			StrBuf.append(c);
			Next++;
		}

		char32_t U8Low = std::stoul(StrBuf);
		if (!(U8Low >= 0xDC00 && U8Low <= 0xDFFF))
			return std::nullopt;
		//计算出原码点
		Out = 0x10000 + (U8 - 0xD800) * 0x400 + (U8Low - 0xDC00);
	}
	else {
		Out = U8;
	}

	std::array<char32_t, 1> OutArrayU32 = { Out };
	std::array<char, 4> OutArrayU8;
	baseee::coder::Utf32ToUtf8(
		OutArrayU32.cbegin(), OutArrayU32.cend(),
		OutArrayU8.begin(), OutArrayU8.end());

	return std::string(OutArrayU8.data());
}

//解析数组
baseee::parser::JsonErrCode 
baseee::parser::JsonParser::ParseValueArray() noexcept {
	Next++;
	bool skip = true;

	JsonData JsonArray;
	JsonArray.JsonT = JsonType::JsonType_Array;
	JsonArray.Data = std::vector<JsonData>();

	auto ErrCode = JsonErrCode::Parse_OK;

	while (true) {
		if (Next == Json.cend()) return JsonErrCode::Parse_MissToken;
		else if (*Next == ']') { Next++; break; }
		else if (*Next == ',') Next++;
		// 在，[xx,xx]中，需要先不对x进行分割(, || ])判断
		else if (skip) skip = false;
		else return JsonErrCode::Parse_MissToken;

		AfterSpace();
		ErrCode = ParseValue();
		if (ErrCode != JsonErrCode::Parse_OK)return ErrCode;

		std::get<std::vector<JsonData>>(JsonArray.Data)
			.push_back(JsonNext);

		AfterSpace();
	}
	
	this->JsonNext = JsonArray;
	return JsonErrCode::Parse_OK;
}

baseee::parser::JsonErrCode
baseee::parser::JsonParser::ParseValueObject() noexcept {
	Next++;
	bool skip = true;

	JsonData JsonObject;
	JsonObject.JsonT = JsonType::JsonType_Object;
	JsonObject.Data = std::multimap<std::string,JsonData>();

	auto ErrCode = JsonErrCode::Parse_OK;
	std::pair<std::string, JsonData> JsonKeyVulanPair;

	while (true) {
		if (Next == Json.cend()) return JsonErrCode::Parse_MissToken;
		else if (*Next == '}') { Next++; break; }
		else if (*Next == ',') Next++;
		// 在，{"xx":xx]中，需要先不对"进行判断
		else if (skip) skip = false;
		else return JsonErrCode::Parse_MissToken;
		
		//读取键
		AfterSpace();
		ErrCode = ParseValue();

		if (ErrCode != JsonErrCode::Parse_OK) 
			return ErrCode;

		if (JsonNext.JsonT != JsonType::JsonType_String)
			return JsonErrCode::Parse_UnknowError;

		JsonKeyVulanPair.first = std::get<std::string>(JsonNext.Data);
		//读取值
		AfterSpace();
		if (*Next != ':') return JsonErrCode::Parse_MissToken;
		else Next++;
		AfterSpace();
		ErrCode = ParseValue();

		if (ErrCode != JsonErrCode::Parse_OK)
			return ErrCode;

		JsonKeyVulanPair.second.JsonT = JsonNext.JsonT;
		JsonKeyVulanPair.second.Data = JsonNext.Data;

		std::get<std::multimap<std::string, JsonData>>(JsonObject.Data)
			.insert(JsonKeyVulanPair);

		AfterSpace();
	}

	this->JsonNext = JsonObject;
	return JsonErrCode::Parse_OK;
}