#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <string>
#include <vector>

#include "PlayListItem.h"

#include <nlohmann/json.hpp>

struct PlayList
{
	PlayList() = default;

	PlayList(std::string const& name):Name(name)
	{ }

	PlayList(nlohmann::json const& json)
	{
		read(json);
	}

	std::vector<PlayListItem> PlayListItems;
	std::string Name;

	void read(const nlohmann::json& json)
	{
		json.at("name").get_to(Name);

		for (auto& item : json.at("items"))
		{
			PlayListItems.emplace_back(item);
		}
	}
};

#endif