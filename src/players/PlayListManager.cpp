#include "PlayListManager.h"

#include "PlayList.h"

#include <filesystem>

#include <fstream>


PlayListManager::PlayListManager():
	//m_scheduleTimer(std::make_unique<QTimer>(this)),
		m_logger(spdlog::get("miniplayer"))
{
	//m_scheduleTimer->setInterval(2000);
	//m_scheduleTimer->moveToThread(&m_scheduleThread);

	//connect(&m_scheduleThread, SIGNAL(started()), m_scheduleTimer.get(), SLOT(start()));
	//connect(m_scheduleTimer.get(), SIGNAL(timeout()), this, SLOT(CheckSchedule()));
	//connect(this, SIGNAL(finished()), m_scheduleTimer.get(), SLOT(stop()));
	//connect(this, SIGNAL(finished()), &m_scheduleThread, SLOT(quit()));
	//m_scheduleThread.start();

	asio::io_context io;

	interval_timer abc{ io, 50ms, [] {
		std::cout << "TEST_ABC" << std::endl;
	} };


	io.run();
}

PlayListManager::~PlayListManager()
{
	//m_scheduleTimer->stop();
	//m_scheduleThread.requestInterruption();
	//m_scheduleThread.quit();
	//m_scheduleThread.wait();
}

bool PlayListManager::LoadPlayLists(std::string const& configFolder)
{
	std::string const filepath = configFolder + "/" + "scottplayer.json";
	if(!std::filesystem::exists(filepath))
	{
		m_logger->warn("config file not found: {}", configFolder);
		return false;
	}
	LoadJsonFile(filepath);
	return true;
}

void PlayListManager::SavePlayLists(std::string const& configFolder)
{
	std::string const filepath = configFolder  + "/" + "scottplayer.json";
	//SaveJsonFile(filepath);
	//MessageSend("Saved: scottplayer.json" );
}

void PlayListManager::PlaySequence(int playlist_index, int sequence_index) const
{
	if (playlist_index < 0 || playlist_index > m_playlists.size())
	{
		return;
	}

	if (sequence_index < 0 || sequence_index > m_playlists.at(playlist_index).PlayListItems.size())
	{
		return;
	}

	//PlaySequenceSend(m_playlists.at(playlist_index).PlayListItems.at(sequence_index).SequenceFile,
	//	m_playlists.at(playlist_index).PlayListItems.at(sequence_index).MediaFile);
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
	if (index < 0 || index >= m_playlists.size())
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

void PlayListManager::EditSchedule(int schedule_index, Schedule schedule)
{
	if (schedule_index < 0 || schedule_index >= m_schedules.size())
	{
		return;
	}
	m_schedules[schedule_index] = std::move(schedule);
	//emit DisplayScheduleSend();
}

void PlayListManager::LoadJsonFile(const std::string& jsonFile)
{
	//using json = nlohmann::json;
	std::ifstream ifs(jsonFile);
	nlohmann::json loadDoc = nlohmann::json::parse(ifs);


	//QFile loadFile(jsonFile);
	//if (!loadFile.open(QIODevice::ReadOnly))
	//{
	//	return;
	//}
	//
	//QByteArray saveData = loadFile.readAll();
	//
	//QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
	//
	ReadPlaylists(loadDoc);
	ReadSchedules(loadDoc);
}

void PlayListManager::ReadPlaylists(nlohmann::json const& json)
{
	m_playlists.clear();
	for (auto& playlist : json.at("playlists")) {
		m_playlists.emplace_back(playlist);
	}
}

void PlayListManager::ReadSchedules(nlohmann::json const& json)
{
	m_schedules.clear();
	for (auto& schedule : json.at("schedules"))	{
		m_schedules.emplace_back(schedule);
	}
}

[[nodiscard]] std::optional< std::reference_wrapper< PlayList const > > PlayListManager::GetPlayList(int index) const
{
	if (index < 0 || index > m_playlists.size())
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

void PlayListManager::CheckSchedule()
{
	if (m_status != PlaybackStatus::Stopped)
	{
		return; 
	}
	//auto const& current = QDateTime::currentDateTime();

	//for (auto const& schedule : m_schedules)
	//{
	//	if (current.date() < schedule.StartDate || current.date() > schedule.EndDate)
	//	{
	//		continue;
	//	}
	//	if (current.time() < schedule.StartTime || current.time() > schedule.EndTime)
	//	{
	//		continue;
	//	}
	//	if (!schedule.Days.contains(QDate::shortDayName(current.date().dayOfWeek()))) 
	//	{
	//		continue;
	//	}
	//	if (schedule.PlayListName == m_currentPlaylist)
	//	{
	//		PlayNextSequence();
	//		break;
	//	}
	//	PlayNewPlaylist(schedule.PlayListName);
	//	break;
	//}
}

void PlayListManager::PlayNextSequence()
{
	if (auto const& playlistRef = GetPlayList(m_currentPlaylist); playlistRef)
	{
		auto const& playlist = playlistRef->get();

		//emit PlaySequenceSend(playlist.PlayListItems[m_nextSequenceIdx].SequenceFile,
		//	playlist.PlayListItems[m_nextSequenceIdx].MediaFile);
		++m_nextSequenceIdx;
		if (m_nextSequenceIdx >= playlist.PlayListItems.size())
		{
			m_nextSequenceIdx = 0;
		}
	}
}

void PlayListManager::PlayNewPlaylist(std::string const& playlistName)
{
	if (auto const& playlistRef = GetPlayList(playlistName); playlistRef)
	{
		auto const& playlist = playlistRef->get();
		m_nextSequenceIdx = 0;
		m_currentPlaylist = playlistName;

		//emit PlaySequenceSend(playlist.PlayListItems[m_nextSequenceIdx].SequenceFile,
		//	playlist.PlayListItems[m_nextSequenceIdx].MediaFile);
		++m_nextSequenceIdx;
		if (m_nextSequenceIdx >= playlist.PlayListItems.size())
		{
			m_nextSequenceIdx = 0;
		}
	}
}
