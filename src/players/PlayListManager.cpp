#include "PlayListManager.h"

#include "PlayList.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <fstream>

namespace
{
	// Short day names as written by the original Qt player, indexed by tm_wday.
	constexpr std::array<char const*, 7> kDayNames{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	// Preferred name first; scottplayer.json is what the original Qt player
	// wrote, kept so existing shows load without renaming anything.
	constexpr std::array<char const*, 2> kConfigFileNames{ "mini_light_player.json", "scottplayer.json" };

	std::tm LocalTimeNow()
	{
		std::time_t const now = std::time(nullptr);
		std::tm local{};
#ifdef _WIN32
		localtime_s(&local, &now);
#else
		localtime_r(&now, &local);
#endif
		return local;
	}
}

PlayListManager::PlayListManager():
		m_logger(spdlog::get("miniplayer"))
{
}

PlayListManager::~PlayListManager() = default;

bool PlayListManager::LoadPlayLists(std::string const& configFolder)
{
	m_configFolder = configFolder;

	for (char const* name : kConfigFileNames)
	{
		std::string const filepath = configFolder + "/" + name;
		if (std::filesystem::exists(filepath))
		{
			m_configFileName = name;
			m_configPath = filepath;
			LoadJsonFile(filepath);

			std::error_code ec;
			m_configLastWrite = std::filesystem::last_write_time(filepath, ec);
			return true;
		}
	}

	m_logger->warn("No {} or {} found in {}",
		kConfigFileNames[0], kConfigFileNames[1], configFolder);
	return false;
}

void PlayListManager::SavePlayLists(std::string const& configFolder)
{
	// Writes back to whichever file was loaded, or the preferred name for a
	// show that had neither yet.
	std::string const filepath = configFolder + "/" + m_configFileName;
	SaveJsonFile(filepath);
}

void PlayListManager::PlaySequence(int playlist_index, int sequence_index)
{
	if (playlist_index < 0 || static_cast<size_t>(playlist_index) >= m_playlists.size())
	{
		return;
	}

	if (sequence_index < 0 || static_cast<size_t>(sequence_index) >= m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	m_currentPlaylist = m_playlists.at(playlist_index).Name;
	m_nextSequenceIdx = sequence_index;
	PlayNextSequence();
}

void PlayListManager::DeleteSequence(int playlist_index, int sequence_index)
{
	if (playlist_index < 0 || static_cast<size_t>(playlist_index) >= m_playlists.size())
	{
		return;
	}

	if (sequence_index < 0 || static_cast<size_t>(sequence_index) >= m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	m_playlists.at(playlist_index).PlayListItems.erase(m_playlists.at(playlist_index).PlayListItems.begin() +sequence_index);
	//emit DisplayPlaylistSend(playlist_index);
}

void PlayListManager::MoveSequenceUp(int playlist_index, int sequence_index)
{
	if (playlist_index < 0 || static_cast<size_t>(playlist_index) >= m_playlists.size())
	{
		return;
	}

	if (sequence_index <= 0 || static_cast<size_t>(sequence_index) >= m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	std::swap(m_playlists.at(playlist_index).PlayListItems.at(sequence_index),
		m_playlists.at(playlist_index).PlayListItems.at(sequence_index - 1));

	//emit DisplayPlaylistSend(playlist_index);
	//emit SelectSequenceSend(sequence_index - 1);
}
void PlayListManager::MoveSequenceDown(int playlist_index, int sequence_index) 
{
	if (playlist_index < 0 || static_cast<size_t>(playlist_index) >= m_playlists.size())
	{
		return;
	}

	if (sequence_index < 0 || sequence_index + 1 >= m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	std::swap(m_playlists.at(playlist_index).PlayListItems.at( sequence_index),
		m_playlists.at(playlist_index).PlayListItems.at(sequence_index + 1));
	
	//emit DisplayPlaylistSend(playlist_index);
	//emit SelectSequenceSend(sequence_index + 1);
}

void PlayListManager::DeletePlayList(int playlist_index)
{
	if (playlist_index < 0 || static_cast<size_t>(playlist_index) >= m_playlists.size())
	{
		return;
	}	

	m_playlists.erase(m_playlists.begin() + playlist_index);

	//redraw
}

void PlayListManager::UpdateStatus(std::string const& sequencePath, PlaybackStatus status)
{
	m_status = status;
}

void PlayListManager::AddPlaylistName(std::string const& playlist)
{
	if (std::any_of(m_playlists.begin(), m_playlists.end(), [&](auto const& elem)
		{ return elem.Name == playlist; })) {
		m_logger->warn("Cannot have Duplicate PlayList Names: {}", playlist);
		return;
	}
	m_playlists.emplace_back(playlist);
}

void PlayListManager::AddSequence(std::string const& fseqPath, std::string const& mediaPath, int index)
{
	if (index < 0 || static_cast<size_t>(index) >= m_playlists.size())
	{
		return;
	}
	m_playlists.at(index).PlayListItems.emplace_back(fseqPath, mediaPath);
	//emit DisplayPlaylistSend(index);
}

void PlayListManager::AddSchedule(Schedule schedule)
{
	m_schedules.emplace_back(std::move(schedule));
	//emit DisplayScheduleSend();
}

void PlayListManager::DeleteSchedule(int schedule_index) 
{
	if (schedule_index < 0 || static_cast<size_t>(schedule_index) >= m_schedules.size())
	{
		return;
	}

	m_schedules.erase(m_schedules.begin() + schedule_index);
	//emit DisplayScheduleSend();
}

void PlayListManager::EditSchedule(int schedule_index, Schedule schedule)
{
	if (schedule_index < 0 || static_cast<size_t>(schedule_index) >= m_schedules.size())
	{
		return;
	}
	m_schedules[schedule_index] = std::move(schedule);
	//emit DisplayScheduleSend();
}

void PlayListManager::MoveScheduleUp(int schedule_index)
{
	if (schedule_index <= 0 || schedule_index >= m_schedules.size())
	{
		return;
	}
	std::swap(m_schedules.at(schedule_index),
		m_schedules.at(schedule_index - 1));
}

void PlayListManager::MoveScheduleDown(int schedule_index)
{
	if (schedule_index < 0 || schedule_index + 1 >= m_schedules.size())
	{
		return;
	}

	std::swap(m_schedules.at(schedule_index),
		m_schedules.at(schedule_index + 1));
}

void PlayListManager::LoadJsonFile(const std::string& jsonFile)
{
	std::ifstream input(jsonFile);
	if (!input.is_open())
	{
		m_logger->warn("Unable to open playlist file: {}", jsonFile);
		return;
	}

	nlohmann::json doc;
	try
	{
		input >> doc;
	}
	catch (nlohmann::json::exception const& ex)
	{
		m_logger->error("Unable to parse {}: {}", jsonFile, ex.what());
		return;
	}

	try
	{
		m_playlists = doc.value("playlists", std::vector<PlayList>{});
		m_schedules = doc.value("schedules", std::vector<Schedule>{});
	}
	catch (nlohmann::json::exception const& ex)
	{
		m_logger->error("Unexpected contents in {}: {}", jsonFile, ex.what());
		return;
	}

	for (auto const& playlist : m_playlists)
	{
		m_logger->info("Loaded playlist '{}' with {} sequences",
			playlist.Name, playlist.PlayListItems.size());
	}
	m_logger->info("Loaded {} schedules", m_schedules.size());
}

void PlayListManager::SaveJsonFile(const std::string& jsonFile)
{
	nlohmann::json doc;
	doc["playlists"] = m_playlists;
	doc["schedules"] = m_schedules;

	std::ofstream output(jsonFile);
	if (!output.is_open())
	{
		m_logger->error("Unable to write playlist file: {}", jsonFile);
		return;
	}

	output << doc.dump(4) << std::endl;
	output.close();
	m_logger->info("Saved: {}", jsonFile);

	// Record our own write, so ReloadIfChanged doesn't immediately reload the
	// file we just wrote from our own in-memory state.
	std::error_code ec;
	m_configLastWrite = std::filesystem::last_write_time(jsonFile, ec);
}

bool PlayListManager::ReloadIfChanged()
{
	// Nothing loaded yet - keep checking the folder in case a config file
	// gets added while the player is running unattended.
	if (m_configPath.empty())
	{
		if (m_configFolder.empty())
		{
			return false;
		}

		for (char const* name : kConfigFileNames)
		{
			std::string const filepath = m_configFolder + "/" + name;
			if (std::filesystem::exists(filepath))
			{
				m_logger->info("Config file appeared, loading: {}", filepath);
				m_configFileName = name;
				m_configPath = filepath;
				LoadJsonFile(filepath);

				std::error_code ec;
				m_configLastWrite = std::filesystem::last_write_time(filepath, ec);
				return true;
			}
		}
		return false;
	}

	std::error_code ec;
	auto const written = std::filesystem::last_write_time(m_configPath, ec);
	if (ec)
	{
		// Probably a transient stat failure (e.g. an editor mid-write, or the
		// file briefly removed while replacing it). Keep the playlists we
		// already have rather than clearing them on a single failed check.
		return false;
	}

	if (written == m_configLastWrite)
	{
		return false;
	}

	m_logger->info("Config file changed on disk, reloading: {}", m_configPath);
	LoadJsonFile(m_configPath);
	m_configLastWrite = written;
	return true;
}

//void PlayListManager::ReadPlaylists(QJsonObject const& json)
//{
//	m_playlists.clear();
//	QJsonArray playlistArray = json["playlists"].toArray();
//	for (auto const& playlist : playlistArray)
//	{
//		QJsonObject playlistObj = playlist.toObject();
//		m_playlists.emplace_back(playlistObj);
//		emit AddPlaylistSend(m_playlists.back().Name, m_playlists.size() - 1 );
//	}
//}

//void PlayListManager::ReadSchedules(QJsonObject const& json)
//{
//	m_schedules.clear();
//	QJsonArray scheduleArray = json["schedules"].toArray();
//	for (auto const& schedule :scheduleArray)
//	{
//		QJsonObject scheduleObj = schedule.toObject();
//		m_schedules.emplace_back(scheduleObj);
//	}
//}
//
//void PlayListManager::WritePlaylists(QJsonObject& json) const
//{
//	QJsonArray playlistArray;
//	for (auto const& playlist : m_playlists)
//	{
//		QJsonObject playlistObj;
//		playlist.write(playlistObj);
//		playlistArray.append(playlistObj);
//	}
//	json["playlists"] = playlistArray;
//}

//void PlayListManager::WriteSchedules(QJsonObject& json) const
//{
//	QJsonArray scheduleArray;
//	for (auto const& schedule : m_schedules)
//	{
//		QJsonObject scheduleObj;
//		schedule.write(scheduleObj);
//		scheduleArray.append(scheduleObj);
//	}
//	json["schedules"] = scheduleArray;
//}

[[nodiscard]] std::optional< std::reference_wrapper< PlayList const > > PlayListManager::GetPlayList(int index) const
{
	if (index < 0 || static_cast<size_t>(index) >= m_playlists.size())
	{
		return std::nullopt;
	}
	return m_playlists.at(index);
}

[[nodiscard]] std::optional< std::reference_wrapper< PlayList const > > PlayListManager::GetPlayList(std::string const& name) const
{
	if (auto const found{ std::find_if(m_playlists.cbegin(),m_playlists.cend(),
											[&name](auto& c) { return c.Name==name; }) };
		found != m_playlists.cend())
	{
		return *found;
	}
	return std::nullopt;
}

std::vector<std::string> PlayListManager::GetPlayLists() const 
{
	std::vector<std::string> playLists;
	std::transform(m_playlists.cbegin(), m_playlists.cend(), std::back_inserter(playLists),
		[](auto const& pl) { return pl.Name; });

	return playLists;
}

std::optional<std::reference_wrapper<Schedule const>> PlayListManager::GetActiveSchedule() const
{
	std::tm const local = LocalTimeNow();

	auto const today = std::chrono::year{ local.tm_year + 1900 } /
		std::chrono::month{ static_cast<unsigned>(local.tm_mon + 1) } /
		std::chrono::day{ static_cast<unsigned>(local.tm_mday) };

	auto const timeOfDay = std::chrono::hours{ local.tm_hour } +
		std::chrono::minutes{ local.tm_min } +
		std::chrono::seconds{ local.tm_sec };

	std::string const dayName = kDayNames.at(local.tm_wday);

	for (auto const& schedule : m_schedules)
	{
		if (schedule.Enabled &&
			schedule.CoversDate(today) &&
			schedule.CoversTime(std::chrono::duration_cast<std::chrono::milliseconds>(timeOfDay)) &&
			schedule.CoversDay(dayName))
		{
			return schedule;
		}
	}
	return std::nullopt;
}

void PlayListManager::CheckSchedule()
{
	if (m_status != PlaybackStatus::Stopped)
	{
		return;
	}

	if (auto const match = GetActiveSchedule(); match)
	{
		Schedule const& schedule = match->get();

		// Already inside this playlist, so just roll on to its next sequence.
		if (schedule.PlayListName == m_currentPlaylist)
		{
			PlayNextSequence();
			return;
		}

		PlayNewPlaylist(schedule.PlayListName);
		return;
	}

	// Nothing is scheduled now, so the next match starts from the top.
	m_currentPlaylist.clear();
	m_nextSequenceIdx = 0;
}

void PlayListManager::StartSequence(PlayListItem const& item)
{
	if (!m_playHandler)
	{
		m_logger->error("No play handler set, cannot start {}", item.SequenceFile);
		return;
	}
	m_status = PlaybackStatus::Playing;
	m_playHandler(item.SequenceFile, item.MediaFile);
}

void PlayListManager::PlayNextSequence()
{
	auto const& playlistRef = GetPlayList(m_currentPlaylist);
	if (!playlistRef)
	{
		return;
	}

	auto const& playlist = playlistRef->get();
	if (playlist.PlayListItems.empty())
	{
		return;
	}

	if (static_cast<size_t>(m_nextSequenceIdx) >= playlist.PlayListItems.size())
	{
		m_nextSequenceIdx = 0;
	}

	StartSequence(playlist.PlayListItems.at(m_nextSequenceIdx));

	++m_nextSequenceIdx;
	if (static_cast<size_t>(m_nextSequenceIdx) >= playlist.PlayListItems.size())
	{
		m_nextSequenceIdx = 0;
	}
}

void PlayListManager::PlayNewPlaylist(std::string const& playlistName)
{
	auto const& playlistRef = GetPlayList(playlistName);
	if (!playlistRef)
	{
		m_logger->warn("Scheduled playlist not found: {}", playlistName);
		return;
	}

	if (playlistRef->get().PlayListItems.empty())
	{
		m_logger->warn("Scheduled playlist '{}' is empty", playlistName);
		return;
	}

	m_logger->info("Starting playlist: {}", playlistName);
	m_currentPlaylist = playlistName;
	m_nextSequenceIdx = 0;
	PlayNextSequence();
}
