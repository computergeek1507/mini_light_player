#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <nlohmann/json.hpp>

#include <string>

struct PlayListItem
{
	std::string SequenceFile;
	std::string MediaFile;

	PlayListItem() = default;

	PlayListItem(std::string const& seq, std::string const& media):
		SequenceFile(seq), MediaFile(media)
	{
	}
};

inline void to_json(nlohmann::json& json, PlayListItem const& item)
{
	json = nlohmann::json{
		{ "seq", item.SequenceFile },
		{ "media", item.MediaFile },
	};
}

inline void from_json(nlohmann::json const& json, PlayListItem& item)
{
	item.SequenceFile = json.value("seq", std::string());
	item.MediaFile = json.value("media", std::string());
}

#endif
