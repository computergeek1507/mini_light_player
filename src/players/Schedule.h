#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <chrono>
#include <string>

struct Schedule
{
	//QString Name;
	std::string PlayListName;
	std::chrono::system_clock::time_point StartTime;
	std::chrono::system_clock::time_point EndTime;
	//QDate StartDate;
	//QDate EndDate;
	std::vector<std::string> Days;

	Schedule() = default;
	~Schedule() = default;

	Schedule(const Schedule &) = default;
    Schedule &operator=(const Schedule &) = default;

	Schedule(std::string const& playlist, std::chrono::system_clock::time_point const& startTime, std::chrono::system_clock::time_point const& endTime, std::vector<std::string> const& days ) :
		PlayListName(playlist), StartTime(startTime), EndTime(endTime), Days(days)
	{

	}

	//Schedule(QJsonObject const& json)
	//{
	//	read(json);
	//}
	//
	//void write(QJsonObject& json) const
	//{
	//	//json["name"] = Name;
	//	json["playList"] = PlayListName;
	//	json["startTime"] = QJsonValue::fromVariant(StartTime);
	//	json["endTime"] = QJsonValue::fromVariant(EndTime);
	//	json["startDate"] = QJsonValue::fromVariant(StartDate);
	//	json["endDate"] = QJsonValue::fromVariant(EndDate);
	//	json["days"] = QJsonValue::fromVariant(Days);
	//}
	//
	//void read(const QJsonObject& json)
	//{
	//	//Name = json["name"].toString();
	//	PlayListName = json["playList"].toString();
	//	StartTime = json["startTime"].toVariant().toTime();
	//	EndTime = json["endTime"].toVariant().toTime();
	//	StartDate = json["startDate"].toVariant().toDate();
	//	EndDate = json["endDate"].toVariant().toDate();
	//	Days = json["days"].toVariant().toStringList();
	//}
};


#endif