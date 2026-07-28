#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

#include "PlayListItem.h"

struct PlayList
{
	PlayList() = default;

	PlayList(std::string const& name):Name(name)
	{ }

	std::vector<PlayListItem> PlayListItems;
	std::string Name;
};

inline void to_json(nlohmann::json& json, PlayList const& playlist)
{
	json = nlohmann::json{
		{ "name", playlist.Name },
		{ "items", playlist.PlayListItems },
	};
}

inline void from_json(nlohmann::json const& json, PlayList& playlist)
{
	playlist.Name = json.value("name", std::string());
	playlist.PlayListItems = json.value("items", std::vector<PlayListItem>{});
}

#endif
