#include "pch.h"

#include <iomanip>
#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>

namespace Dlight
{
	const char* Logger::LevelTag(LogLevel level)
	{
		switch (level)
		{
		case LogLevel::Info:    return "INFO ";
		case LogLevel::Warning: return "WARN ";
		case LogLevel::Error:   return "ERROR";
		}

		return "?????";
	}

	const char* Logger::FileNameOnly(const char* path)
	{
		const char* name = path;

		for (const char* cursor = path; *cursor != '\0'; ++cursor)
		{
			if (*cursor == '\\' || *cursor == '/')
			{
				name = cursor + 1;
			}
		}

		return name;
	}

	std::string Logger::Timestamp()
	{
		using namespace std::chrono;

		const auto now = system_clock::now();
		const auto seconds = system_clock::to_time_t(now);
		const auto millis = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;

		std::tm local{};
		localtime_s(&local, &seconds);

		std::ostringstream stream;
		stream << std::put_time(&local, "%H:%M:%S")
			<< '.' << std::setfill('0') << std::setw(3) << millis;

		return stream.str();
	}

	void Logger::Write(LogLevel level, const char* filePath, int line, const std::string& message)
	{
		// 함수 로컬 static 은 C++11 부터 초기화가 스레드 안전하게 보장된다.
		static std::mutex mutex;
		static std::ofstream logFile("log.txt", std::ios::out | std::ios::trunc);

		std::ostringstream stream;
		stream << '[' << Timestamp() << "][" << LevelTag(level) << "] "
			<< FileNameOnly(filePath) << ':' << line << " | " << message;

		const std::string formatted = stream.str();

		// 포맷팅은 락 밖에서 끝내고, 출력만 잠근다.
		const std::lock_guard<std::mutex> lock(mutex);

		std::ostream& console = (level == LogLevel::Error) ? std::cerr : std::cout;
		console << formatted << '\n';

		if (logFile.is_open())
		{
			logFile << formatted << '\n';

			// 매 메시지마다 flush 하면 병목이 되므로 Error 만 즉시 기록한다.
			if (level == LogLevel::Error)
			{
				logFile.flush();
			}
		}
	}
}
