#include "miniplayer.h"

#include "spdlog/spdlog.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <filesystem>
#include <utility>

MiniPlayer::MiniPlayer(std::string showfolder): m_showfolder(std::move(showfolder))
{
	auto const log_name{ "log.txt" };

	//m_appdir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	//std::filesystem::create_directory(m_appdir.toStdString());
	//std::String logdir = m_appdir + "/log/";
	//std::filesystem::create_directory(logdir.toStdString());

	try
	{
		auto file{ log_name };
		std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(file, 1024 * 1024, 5, false));

		m_logger = std::make_shared<spdlog::logger>("miniplayer", sinks.begin(), sinks.end());
		m_logger->flush_on(spdlog::level::debug);
		m_logger->set_level(spdlog::level::debug);
		m_logger->set_pattern("[%D %H:%M:%S] [%L] %v");
		spdlog::register_logger(m_logger);
	}
	catch (std::exception& /*ex*/)
	{
		
	}


	m_player = std::make_unique<SequencePlayer>();
	m_playlists = std::make_unique<PlayListManager>();	
	m_player->LoadConfigs(m_showfolder);
	m_playlists->LoadPlayLists(m_showfolder);
	//m_showfolder = lastfolder;


}