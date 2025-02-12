#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <string>
#include <nlohmann/json.hpp>

struct PlayListItem
{
	std::string SequenceFile;
	std::string MediaFile;

	PlayListItem() = default;

	PlayListItem(std::string const& seq, std::string const& media):
		SequenceFile(seq), MediaFile(media)
	{
	}

	explicit PlayListItem(nlohmann::json const& json)
	{
		read(json);
	}
	//
	//void write(QJsonObject& json) const
	//{
	//	json["seq"] = SequenceFile;
	//	json["media"] = MediaFile;
	//}
	//
	void read(nlohmann::json const& json)
	{
		json.at("seq").get_to(SequenceFile);
		json.at("media").get_to(MediaFile);
	}
};

#endif