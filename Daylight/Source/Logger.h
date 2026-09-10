#pragma once

#include <sstream>
#include <string>
#include <utility>

namespace Dlight
{
	enum class LogLevel
	{
		Info,
		Warning,
		Error,
	};

	class Logger
	{
	public:
		// 정적 유틸리티 클래스이므로 인스턴스를 만들지 않는다.
		Logger() = delete;

		template <typename... Args>
		static void Log(LogLevel level, const char* filePath, int line, Args&&... args)
		{
			std::ostringstream stream;
			(stream << ... << std::forward<Args>(args));
			Write(level, filePath, line, stream.str());
		}

	private:
		static void Write(LogLevel level, const char* filePath, int line, const std::string& message);

		static const char* LevelTag(LogLevel level);
		static const char* FileNameOnly(const char* path);
		static std::string Timestamp();
	};
}

// __FILE__ / __LINE__ 을 호출 지점에서 잡아야 하므로 매크로가 필요하다.
#define DL_LOG_INFO(...)  ::Dlight::Logger::Log(::Dlight::LogLevel::Info,    __FILE__, __LINE__, __VA_ARGS__)
#define DL_LOG_WARN(...)  ::Dlight::Logger::Log(::Dlight::LogLevel::Warning, __FILE__, __LINE__, __VA_ARGS__)
#define DL_LOG_ERROR(...) ::Dlight::Logger::Log(::Dlight::LogLevel::Error,   __FILE__, __LINE__, __VA_ARGS__)
