#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

// Times are stored as an offset from midnight rather than a time_point, since
// a schedule describes a time of day that recurs, not a single instant.
struct Schedule
{
	std::string PlayListName;
	bool Enabled{ true };
	std::chrono::year_month_day StartDate{ std::chrono::year{ 2020 } / 1 / 1 };
	std::chrono::year_month_day EndDate{ std::chrono::year{ 2099 } / 12 / 31 };
	std::chrono::milliseconds StartTime{ 0 };
	std::chrono::milliseconds EndTime{ std::chrono::hours{ 24 } };
	std::vector<std::string> Days;

	Schedule() = default;
	~Schedule() = default;

	Schedule(const Schedule&) = default;
	Schedule& operator=(const Schedule&) = default;

	Schedule(std::string const& playlist,
		std::chrono::milliseconds const& startTime,
		std::chrono::milliseconds const& endTime,
		std::vector<std::string> const& days) :
		PlayListName(playlist), StartTime(startTime), EndTime(endTime), Days(days)
	{
	}

	// Windows that end before they start are treated as running past midnight,
	// which is normal for a Christmas show.
	[[nodiscard]] bool CoversTime(std::chrono::milliseconds timeOfDay) const
	{
		if (StartTime <= EndTime)
		{
			return timeOfDay >= StartTime && timeOfDay <= EndTime;
		}
		return timeOfDay >= StartTime || timeOfDay <= EndTime;
	}

	[[nodiscard]] bool CoversDate(std::chrono::year_month_day date) const
	{
		using days = std::chrono::sys_days;
		return days{ date } >= days{ StartDate } && days{ date } <= days{ EndDate };
	}

	[[nodiscard]] bool CoversDay(std::string const& shortDayName) const
	{
		// An empty day list means every day.
		return Days.empty() ||
			std::find(Days.cbegin(), Days.cend(), shortDayName) != Days.cend();
	}
};

namespace ScheduleJson
{
	// "YYYY-MM-DD", as written by the original Qt player.
	inline std::chrono::year_month_day ParseDate(std::string const& text,
		std::chrono::year_month_day fallback)
	{
		int y{ 0 }, m{ 0 }, d{ 0 };
		if (sscanf(text.c_str(), "%d-%d-%d", &y, &m, &d) != 3)
		{
			return fallback;
		}
		auto const parsed = std::chrono::year{ y } / m / d;
		return parsed.ok() ? parsed : fallback;
	}

	inline std::string FormatDate(std::chrono::year_month_day date)
	{
		char buf[16]{};
		snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
			static_cast<int>(date.year()),
			static_cast<unsigned>(date.month()),
			static_cast<unsigned>(date.day()));
		return buf;
	}

	// "HH:MM:SS.mmm", also accepting "HH:MM" and "HH:MM:SS".
	inline std::chrono::milliseconds ParseTime(std::string const& text,
		std::chrono::milliseconds fallback)
	{
		int h{ 0 }, m{ 0 }, s{ 0 }, ms{ 0 };
		int const fields = sscanf(text.c_str(), "%d:%d:%d.%d", &h, &m, &s, &ms);
		if (fields < 2)
		{
			return fallback;
		}
		return std::chrono::hours{ h } + std::chrono::minutes{ m } +
			std::chrono::seconds{ s } + std::chrono::milliseconds{ ms };
	}

	inline std::string FormatTime(std::chrono::milliseconds time)
	{
		auto const total = time.count();
		char buf[24]{};
		snprintf(buf, sizeof(buf), "%02lld:%02lld:%02lld.%03lld",
			total / 3600000, (total / 60000) % 60, (total / 1000) % 60, total % 1000);
		return buf;
	}
}

inline void to_json(nlohmann::json& json, Schedule const& schedule)
{
	json = nlohmann::json{
		{ "playList", schedule.PlayListName },
		{ "enabled", schedule.Enabled },
		{ "startDate", ScheduleJson::FormatDate(schedule.StartDate) },
		{ "endDate", ScheduleJson::FormatDate(schedule.EndDate) },
		{ "startTime", ScheduleJson::FormatTime(schedule.StartTime) },
		{ "endTime", ScheduleJson::FormatTime(schedule.EndTime) },
		{ "days", schedule.Days },
	};
}

inline void from_json(nlohmann::json const& json, Schedule& schedule)
{
	Schedule const defaults;

	schedule.PlayListName = json.value("playList", std::string());
	schedule.Enabled = json.value("enabled", true);
	schedule.StartDate = ScheduleJson::ParseDate(json.value("startDate", std::string()), defaults.StartDate);
	schedule.EndDate = ScheduleJson::ParseDate(json.value("endDate", std::string()), defaults.EndDate);
	schedule.StartTime = ScheduleJson::ParseTime(json.value("startTime", std::string()), defaults.StartTime);
	schedule.EndTime = ScheduleJson::ParseTime(json.value("endTime", std::string()), defaults.EndTime);
	schedule.Days = json.value("days", std::vector<std::string>{});
}

#endif
