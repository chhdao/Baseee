/* * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 这个文件来自 GOSCPS(https://github.com/GOSCPS)
 * 使用 GOSCPS 许可证
 * File:    log.cpp
 * Content: log Source File
 * Copyright (c) 2020 GOSCPS 保留所有权利.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <mutex>
#include <sstream>
#include <thread>
#include <sstream>
#include <ctime>
#include <exception>
#include <string_view>
#include "log.hpp"


void baseee::log::logger::PrintLog(std::string_view&& log) noexcept {
	_PrintLog(this->DefaultOutLevel, std::forward<std::string_view&&>(log));
	return;
}

void baseee::log::logger::_PrintLog(LogLevel level,std::string_view&& log) noexcept {
	this->Mutex.lock();
	{
		auto Lowest = static_cast<std::underlying_type<LogLevel>::type>(this->LowestLevelOutStream);
		auto Origin = static_cast<std::underlying_type<LogLevel>::type>(level);

		if (Origin >= Lowest && this->OutStream.good()) {
			std::string OutLog;

				OutLog = std::string(this->GetFormat(baseee::log::ToString(level), this->LogFormat));
				OutLog += log;
				OutLog += '\n';

			this->OutStream.write(OutLog.c_str(), OutLog.size());
			this->OutStream.flush();
		}
	}

	{
		auto Lowest = static_cast<std::underlying_type<LogLevel>::type>(this->LowestLevelOutFile);
		auto Origin = static_cast<std::underlying_type<LogLevel>::type>(level);

		if (Origin >= Lowest && this->OutFile.is_open()) {
			std::string OutLog;

			OutLog = std::string(this->GetFormat(baseee::log::ToString(level), this->LogFormat));
			OutLog += log;
			OutLog += '\n';

			this->OutFile.write(OutLog.c_str(), OutLog.size());
			this->OutFile.flush();
		}
	}
	this->Mutex.unlock();
	return;
}


std::string baseee::log::logger::GetFormat(
	const std::string_view &level,
	const std::string_view &format) noexcept {
	auto t = std::time(0);
	std::tm tms;

#if WIN32
	//For Windows
	localtime_s(&tms,&t);
#else
	//For Linux
	localtime_r(&t, &tms);
#endif

	std::tm* tm = &tms;

	std::string Out(format);

	while (true) {

		if (Out.find("{year}") != Out.npos) {
			Out.replace(Out.find("{year}"),
				sizeof "{year}"-1,
				std::to_string(tm->tm_year + 1900));
		}

		else if (Out.find("{month}") != Out.npos) {
			Out.replace(Out.find("{month}"),
				sizeof "{month}"-1,
				std::to_string(tm->tm_mon + 1));
		}

		else if (Out.find("{day}") != Out.npos) {
			Out.replace(Out.find("{day}"),
				sizeof "{day}"-1,
				std::to_string(tm->tm_mday));
		}

		else if (Out.find("{hour}") != Out.npos) {
			Out.replace(Out.find("{hour}"),
				sizeof "{year}"-1,
				std::to_string(tm->tm_hour + 1));
		}

		else if (Out.find("{min}") != Out.npos) {
			Out.replace(Out.find("{min}"),
				sizeof "{min}"-1,
				std::to_string(tm->tm_min));
		}

		else if (Out.find("{sec}") != Out.npos) {
			Out.replace(Out.find("{sec}"),
				sizeof "{sec}"-1,
				std::to_string(tm->tm_sec));
		}

		else if (Out.find("{threadId}") != Out.npos) {
			std::ostringstream s;
			s << std::this_thread::get_id();
			Out.replace(Out.find("{threadId}"),
				sizeof "{threadId}"-1,
				s.str());
		}

		else if (Out.find("{level}") != Out.npos) {
			Out.replace(Out.find("{level}"),
				sizeof "{level}"-1,
				level);
		}

		else if (Out.find("{count}") != Out.npos) {
			Out.replace(Out.find("{count}"),
				sizeof "{count}" - 1,
				std::to_string(Counter));
		}

		else break;
	}
	return Out;
}

void baseee::log::logger::OpenFile(std::string_view &&f) {
	if (f == "") 
		return;

	std::string s(GetFormat("",f));

	this->OutFile.open(s, std::ios::out);

	if (!this->OutFile) {
		throw std::runtime_error("Open File Error" + s);
	}

	return;
}