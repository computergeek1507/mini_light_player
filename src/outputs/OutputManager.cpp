#include "OutputManager.h"

#include "ArtNetOutput.h"
#include "DDPOutput.h"
#include "E131Output.h"

#include "tinyxml2.h"

#include <algorithm>
#include <exception>
#include <filesystem>

namespace
{
	// tinyxml2 returns nullptr for a missing attribute; constructing a
	// std::string from that is undefined behaviour.
	std::string AttrOr(tinyxml2::XMLElement const* element, char const* name, std::string const& fallback)
	{
		char const* const value = element->Attribute(name);
		if (value == nullptr || *value == '\0')
		{
			return fallback;
		}
		return value;
	}

	uint64_t AttrNumOr(tinyxml2::XMLElement const* element, char const* name, uint64_t fallback)
	{
		std::string const value = AttrOr(element, name, std::string());
		if (value.empty())
		{
			return fallback;
		}
		try
		{
			return std::stoull(value);
		}
		catch (std::exception const&)
		{
			return fallback;
		}
	}
}

OutputManager::OutputManager():
		m_logger(spdlog::get("miniplayer"))
{

}

bool OutputManager::OpenOutputs()
{
	int opened{ 0 };
	int failed{ 0 };
	for (auto const& o : m_outputs)
	{
		if (!o->Enabled)
		{
			continue;
		}
		if (o->Open())
		{
			++opened;
		}
		else
		{
			++failed;
			m_logger->warn("Failed to open output {}", o->IP);
		}
	}
	m_logger->info("Opened {} of {} outputs", opened, opened + failed);
	return opened > 0;
}

void OutputManager::CloseOutputs()
{
	for (auto const& o : m_outputs)
	{
		o->Close();
	}
}

void OutputManager::OutputData(uint8_t* data)
{
	//TODO: multithread
	for (auto const& o : m_outputs)
	{
		o->OutputFrame(data);
	}
}

bool OutputManager::LoadOutputs(std::string const& outputConfig)
{
	using namespace tinyxml2;
	XMLDocument xmlNetworks;
	
	// Load the XML file into the Doc instance
	auto work = xmlNetworks.LoadFile(outputConfig.c_str());
	if (work != XML_SUCCESS)
	{
		m_logger->warn("Failed to read XML");
		return false;
	}
	// Get root Element
	XMLElement * rootXML = xmlNetworks.RootElement();

	if (!rootXML)
	{
		m_logger->warn("Failed to read XML");
		return false;
	}
	
	uint64_t startChannel{ 1 };
	XMLElement * controllerXML = rootXML->FirstChildElement("Controller"); 

	while(controllerXML)
	{
		// xLights writes "Active", "Inactive" or "xLights Only". Only "Inactive"
		// means do not send. Note this must compare the string, not the pointer.
		std::string const activeState = AttrOr(controllerXML, "ActiveState", "Active");
		bool const active = (activeState != "Inactive");

		std::string const name = AttrOr(controllerXML, "Name", "<unnamed>");
		XMLElement * networkXML = controllerXML->FirstChildElement("network");

		while(networkXML)
		{
			std::string const nType = AttrOr(networkXML, "NetworkType", std::string());
			std::string const ipAddress = AttrOr(networkXML, "ComPort", std::string());
			uint64_t const iChannels = AttrNumOr(networkXML, "MaxChannels", 0);
			uint32_t const universe = static_cast<uint32_t>(AttrNumOr(networkXML, "BaudRate", 1));

			if ("DDP" == nType)
			{
				auto ddp = std::make_unique<DDPOutput>();
				ddp->IP = ipAddress;
				ddp->PacketSize = static_cast<uint16_t>(AttrNumOr(networkXML, "ChannelsPerPacket", 1440));
				ddp->KeepChannels = AttrNumOr(networkXML, "KeepChannelNumbers", 1) != 0;
				ddp->StartChannel = startChannel;
				ddp->Channels = iChannels;
				ddp->Enabled = active;
				m_outputs.push_back(std::move(ddp));

				m_logger->debug("Adding Output '{}' type: {} ip: {} channels: {}-{}",
					name, nType, ipAddress, startChannel, startChannel + iChannels - 1);
			}
			else if ("E131" == nType || "ArtNet" == nType)
			{
				// A single xLights network row can span several universes. Each one
				// is a separate packet on the wire, so it needs its own output.
				uint64_t const perUniverse = std::min<uint64_t>(iChannels, 512);
				uint64_t remaining = iChannels;
				uint32_t currentUniverse = universe;
				uint64_t currentChannel = startChannel;

				while (remaining > 0)
				{
					uint16_t const packetSize = static_cast<uint16_t>(std::min(remaining, perUniverse));

					if ("E131" == nType)
					{
						auto e131 = std::make_unique<E131Output>();
						e131->IP = ipAddress;
						e131->PacketSize = packetSize;
						e131->Universe = currentUniverse;
						e131->StartChannel = currentChannel;
						e131->Channels = packetSize;
						e131->Enabled = active;
						m_outputs.push_back(std::move(e131));
					}
					else
					{
						auto artnet = std::make_unique<ArtNetOutput>();
						artnet->IP = ipAddress;
						artnet->PacketSize = packetSize;
						artnet->Universe = currentUniverse;
						artnet->StartChannel = currentChannel;
						artnet->Channels = packetSize;
						artnet->Enabled = active;
						m_outputs.push_back(std::move(artnet));
					}

					m_logger->debug("Adding Output '{}' type: {} ip: {} universe: {} channels: {}-{}",
						name, nType, ipAddress, currentUniverse, currentChannel, currentChannel + packetSize - 1);

					remaining -= packetSize;
					currentChannel += packetSize;
					++currentUniverse;
				}
			}
			else
			{
				m_logger->warn("Unsupported output type '{}' on controller '{}', {} channels will be skipped",
					nType, name, iChannels);
			}
			startChannel += iChannels;

			networkXML = networkXML->NextSiblingElement("network");
		}

		controllerXML = controllerXML->NextSiblingElement("Controller");
	}

	m_channelCount = (startChannel - 1);
	m_logger->info("Loaded {} outputs, {} channels total", m_outputs.size(), m_channelCount);
	return true;
}