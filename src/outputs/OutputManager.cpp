#include "OutputManager.h"

#include "ArtNetOutput.h"
#include "DDPOutput.h"
#include "E131Output.h"

#include "tinyxml2.h"
#include <filesystem>

OutputManager::OutputManager():
		m_logger(spdlog::get("miniplayer"))
{

}

bool OutputManager::OpenOutputs()
{
	for (auto const& o : m_outputs)
	{
		o->Open();
	}
	return true;
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
		bool const active = controllerXML->Attribute("ActiveState", "Active") == "Active";
		XMLElement * networkXML = controllerXML->FirstChildElement("network");
		std::string const name = controllerXML->Attribute("Name");

		while(networkXML)
		{			
			std::string const nType = networkXML->Attribute("NetworkType");
			 
			std::string sChannels = networkXML->Attribute("MaxChannels");
			if(sChannels.empty()) sChannels = "0";
			std::string const ipAddress = networkXML->Attribute("ComPort");
			std::string const universe = networkXML->Attribute("BaudRate");
			uint64_t iChannels = std::stoull(sChannels);

			m_logger->debug("Adding Output '{}' type: {}", name, nType);

			if ("DDP" == nType)
			{
				std::string sKeepChannels = networkXML->Attribute("KeepChannelNumbers", "1");
				if(sKeepChannels.empty()) sKeepChannels = "1";
				std::string sPacketSize = networkXML->Attribute("ChannelsPerPacket");
				if(sPacketSize.empty()) sPacketSize = "1440";
	
				auto ddp = std::make_unique<DDPOutput>();
				ddp->IP = ipAddress;
				ddp->PacketSize = std::stoul( sPacketSize);
				ddp->KeepChannels = std::stoi(sPacketSize);
				ddp->StartChannel = startChannel;
				ddp->Channels = iChannels;
				ddp->Enabled = active;
				m_outputs.push_back(std::move(ddp));
			}
			else if ("E131" == nType)
			{
				std::string sPacketSize = networkXML->Attribute("MaxChannels");
				if(sPacketSize.empty()) sPacketSize = "510";
				auto e131 = std::make_unique<E131Output>();
				e131->IP = ipAddress;
				e131->PacketSize = std::stoul( sPacketSize);
				e131->Universe = std::stoul(universe);
				e131->StartChannel = startChannel;
				e131->Channels = iChannels;//todo fix
				e131->Enabled = active;
				m_outputs.push_back(std::move(e131));
			}
			else if ("ArtNet" == nType)
			{
				std::string sPacketSize = networkXML->Attribute("MaxChannels");
				if(sPacketSize.empty()) sPacketSize = "510";
				auto artnet = std::make_unique<ArtNetOutput>();
				artnet->IP = ipAddress;
				artnet->PacketSize =std::stoul( sPacketSize);
				artnet->Universe = std::stoul(universe);
				artnet->StartChannel = startChannel;
				artnet->Channels = iChannels;//todo fix
				artnet->Enabled = active;
				m_outputs.push_back(std::move(artnet));
			}
			else
			{
				m_logger->warn("Unsupported output type: {}", nType);
				//unsupported type
			}
			startChannel += iChannels;

			networkXML = networkXML->NextSiblingElement("network");
		}

		controllerXML = controllerXML->NextSiblingElement("Controller");
	}

	m_channelCount = (startChannel - 1);
	return true;
}