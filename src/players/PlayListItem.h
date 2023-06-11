#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

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

	//explicit PlayListItem(QJsonObject const& json)
	//{
	//	read(json);
	//}
	//
	//void write(QJsonObject& json) const
	//{
	//	json["seq"] = SequenceFile;
	//	json["media"] = MediaFile;
	//}
	//
	//void read(const QJsonObject& json)
	//{
	//	SequenceFile = json["seq"].toString();
	//	MediaFile = json["media"].toString();
	//}
};

#endif