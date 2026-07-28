#include "SequencePlayer.h"

//#include <QDir>
//#include <QCoreApplication>

#include "filesystem"

SequencePlayer::SequencePlayer():
	//m_mediaPlayer(std::make_unique<QMediaPlayer>()),
	//m_playbackThread(std::make_unique<QThread>(this)),
	//m_playbackTimer(std::make_unique<QTimer>(this)),
	m_logger(spdlog::get("miniplayer"))
{
	memset(m_seqData, 0, sizeof(m_seqData));

	//m_playbackTimer = std::make_unique<QTimer>(this);
	//m_playbackTimer->setTimerType(Qt::PreciseTimer);
	//m_playbackTimer->setInterval(50);

	//m_playbackThread = std::make_unique<QThread>(this);
	//moveToThread(&m_playbackThread);
	//m_playbackTimer->moveToThread(&m_playbackThread);
	//this->moveToThread(thread);
	//m_playbackTimer->moveToThread(&m_playbackThread);

	//connect(&m_playbackThread, SIGNAL(started()), m_playbackTimer.get(), SLOT(start()));
	//connect(m_playbackTimer.get(), SIGNAL(timeout()), this, SLOT(TriggerOutputData()));
	//connect(this, SIGNAL(finished()), m_playbackTimer.get(), SLOT(stop()));
	//connect(this, SIGNAL(finished()), &m_playbackThread, SLOT(quit()));


	//m_mediaPlayer = std::make_unique<QMediaPlayer>();
	//connect(m_mediaPlayer.get(), &QMediaPlayer::positionChanged, this, &SequencePlayer::TriggerTimedOutputData);
	//m_mediaPlayer->setVolume(100);

	//connect(m_mediaPlayer.get(), &QMediaPlayer::mediaStatusChanged,	this, &SequencePlayer::MediaStatusChanged);
	//connect(m_mediaPlayer.get(), &QMediaPlayer::stateChanged,	this, &SequencePlayer::MediaStateChanged);

	m_syncManager = std::make_unique<SyncManager>();
}

SequencePlayer::~SequencePlayer()
{
	//m_playbackTimer->stop();
	//m_playbackThread.requestInterruption();
	//m_playbackThread.quit();
	//m_playbackThread.wait();
}

void SequencePlayer::LoadConfigs(std::string const& configPath)
{
	LoadOutputs(configPath + "/" + "xlights_networks.xml");
}

void SequencePlayer::LoadSequence(std::string const& sequencePath, std::string const& mediaPath)
{
	//emit UpdatePlaybackStatus(sequencePath, PlaybackStatus::Loading);
	bool loaded = LoadSeqFile(sequencePath);

	if(!loaded)
	{
		m_logger->error("Unable to load sequence file: {}", sequencePath);
		//failed to load
		return;
	}
	//QFileInfo seqInfo(sequencePath);
	m_seqFileName = std::filesystem::path( sequencePath ).filename().string();

	if(!mediaPath.empty())
	{
		m_mediaFile = mediaPath;
		m_mediaName = std::filesystem::path( mediaPath ).filename().string();
	}

	if(!m_mediaFile.empty() && std::filesystem::exists(m_mediaFile))
	{
		m_seqType = SeqType::Music;
	}
	else
	{
		m_seqType = SeqType::Animation;
	}
	PlaySequence();
}

void SequencePlayer::PlaySequence()
{
	if (!m_outputManager->OpenOutputs())
	{
		m_logger->error("No outputs could be opened, not playing {}", m_seqFileName);
		return;
	}
	m_syncManager->OpenOutputs();

	m_lastFrameRead = 0;
	m_lastFrameData.reset(m_seqFile->getFrame(m_lastFrameRead));
	m_playing = true;

	m_logger->info("Playing {} - {} frames at {}ms ({}s)",
		m_seqFileName, m_numberofFrame, m_seqStepTime, m_seqMSDuration / 1000);

	if(SeqType::Animation == m_seqType)
	{
		StartAnimationSeq();
	}
	else if(SeqType::Music == m_seqType)
	{
		StartMusicSeq();
	}
}

void SequencePlayer::StopSequence()
{
	//m_playbackTimer->stop();
	//m_playbackThread.requestInterruption();
	//m_playbackThread.quit();
	//m_playbackThread.wait();
	//
	//
	//if(m_mediaPlayer->state() == QMediaPlayer::PlayingState)
	//{
	//	m_mediaPlayer->stop();
	//}

	if (!m_playing.exchange(false))
	{
		return;
	}

	m_syncManager->SendStop();

	m_outputManager->CloseOutputs();
	m_syncManager->CloseOutputs();

	m_logger->info("Sequence ended: {}", m_seqFileName);
	//emit UpdateStatus("Sequence Ended " + m_seqFileName);
	//emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
}

void SequencePlayer::TriggerOutputData()
{
	//int64_t timeMS = m_lastFrameRead * m_seqStepTime;

	//qDebug() << "O:" << timeMS << "ms";
	if (m_lastFrameData == nullptr)
	{
		return;
	}
	m_lastFrameData->readFrame((uint8_t*)m_seqData, FPPD_MAX_CHANNELS);
	m_outputManager->OutputData((uint8_t*)m_seqData);
	SendSync(m_lastFrameRead);
	m_lastFrameRead++;

	//if((m_lastFrameRead * m_seqStepTime) % 1000 == 0)
	//{
	//	//emit UpdateTime(m_seqFileName, m_lastFrameRead * m_seqStepTime, m_seqMSDuration );
	//}
	//
	if(m_lastFrameRead >= m_numberofFrame)
	{
		StopSequence();
		return;
	}
	m_lastFrameData.reset(m_seqFile->getFrame(m_lastFrameRead));
}

void SequencePlayer::TriggerTimedOutputData(uint32_t timeMS)
{
	//if(m_mediaPlayer->state() != QMediaPlayer::PlayingState)
	//{
	//	return;
	//}
	int64_t approxFrame = timeMS / m_seqStepTime;

	if (approxFrame >= m_numberofFrame)
	{
		StopSequence();
		return;
	}

	//qDebug() << "O:" << timeMS << "ms";
	m_lastFrameData.reset(m_seqFile->getFrame(approxFrame));
	if (m_lastFrameData == nullptr)
	{
		return;
	}
	m_lastFrameData->readFrame((uint8_t*)m_seqData, FPPD_MAX_CHANNELS);
	m_outputManager->OutputData((uint8_t*)m_seqData);
	
	SendSync(approxFrame);
	//if ((approxFrame * m_seqStepTime) % 1000 == 0)
	//{
		//emit UpdateTime(m_seqFileName, approxFrame * m_seqStepTime, m_seqMSDuration);
	//}

	
	//m_lastFrameData = m_seqFile->getFrame(m_lastFrameRead);
}

void SequencePlayer::LoadOutputs(std::string const& configPath)
{
	m_outputManager = std::make_unique<OutputManager>();
	if(m_outputManager->LoadOutputs(configPath))
	{
		//emit UpdateStatus("Loaded: " + configPath);
	}
}

void SequencePlayer::SendSync(uint32_t frameIdx)
{
	m_syncManager->SendSync(m_seqStepTime, frameIdx, m_seqFileName, m_mediaName);
}

bool SequencePlayer::LoadSeqFile(std::string const& sequencePath)
{
	// Releases any previously loaded sequence.
	m_lastFrameData.reset();
	m_seqFile.reset(FSEQFile::openFSEQFile(sequencePath));
	if (m_seqFile == nullptr)
	{
		//emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
		return false;
	}
	m_seqStepTime = m_seqFile->getStepTime();

	m_mediaFile = m_seqFile->getMediaFilename();

	m_seqMSDuration = m_seqFile->getNumFrames() * m_seqFile->getStepTime();
	m_numberofFrame = m_seqFile->getNumFrames();
	return true;
}

void SequencePlayer::StartAnimationSeq()
{
	//m_playbackTimer = std::make_unique<QTimer>(this);
    //m_playbackTimer->setTimerType(Qt::PreciseTimer);
    //m_playbackTimer->setInterval(m_seqStepTime);

    //m_playbackThread = std::make_unique<QThread>(this);
    //moveToThread(&m_playbackThread);
    //m_playbackTimer->moveToThread(&m_playbackThread);
    //this->moveToThread(thread);
	//m_playbackTimer->moveToThread( &m_playbackThread );

    //connect(&m_playbackThread, SIGNAL(started()), m_playbackTimer.get(), SLOT(start()));
    //connect(m_playbackTimer.get(), SIGNAL(timeout()), this, SLOT(TriggerOutputData()));
    //connect(this, SIGNAL(finished()), m_playbackTimer.get(), SLOT(stop()));
    //connect(this, SIGNAL(finished()), &m_playbackThread, SLOT(quit()));

	//m_playbackTimer->setInterval(m_seqStepTime);
	//emit UpdateStatus("Playing " + m_seqFileName);
	//emit UpdatePlaybackStatus(m_seqFileName, PlaybackStatus::Playing);
	//m_playbackThread.start();
}

void SequencePlayer::StartMusicSeq()
{
	//if(!QFile::exists(m_mediaFile))
	//{
	//	m_logger->error("Unable to find media file: {}", m_mediaFile.toStdString());
	//	emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
	//	return;
	//}
	//m_mediaPlayer->setNotifyInterval(m_seqStepTime);
	//m_mediaPlayer->setMedia(QUrl::fromLocalFile(m_mediaFile));
	////auto test = m_mediaPlayer->mediaStatus();
	//int count{100};
	//while (m_mediaPlayer->mediaStatus() == QMediaPlayer::LoadingMedia && count > 0)
	//{
	//	QCoreApplication::processEvents();
	//	QThread::msleep(10);
	//	--count;
	//}
	////
	//if (m_mediaPlayer->mediaStatus() != QMediaPlayer::LoadedMedia)
	//{
	//	m_logger->error("Unable to Load media file{}: {}", m_mediaPlayer->mediaStatus(), m_mediaFile.toStdString());
	//	emit UpdatePlaybackStatus("", PlaybackStatus::Stopped);
	//	return;
	//}
	////connect(&player, &QMediaPlayer::mediaStatusChanged,
	////	this, [&](QMediaPlayer::MediaStatus status) {
	////		if (status == QMediaPlayer::LoadedMedia) playClicked();
	////	});
	////m_mediaPlayer->setNotifyInterval(m_seqStepTime);
	//emit UpdateStatus("Playing " + m_seqFileName);
	//emit UpdatePlaybackStatus(m_seqFileName, PlaybackStatus::Playing);
	//m_mediaPlayer->play();
}



void SequencePlayer::SetMultisync(bool enabled)
{
	m_syncManager->SetEnabled(enabled);
}