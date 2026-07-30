#include "miniplayer.h"

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

namespace
{
	void PrintUsage(char const* exe)
	{
		printf("mini_light_player - fseq show player\n\n");
		printf("Usage: %s <show_folder> [sequence.fseq] [media_file] [--multisync]\n\n", exe);
		printf("  show_folder   xLights show directory holding xlights_networks.xml\n");
		printf("  sequence.fseq optional sequence to play immediately; may be a path\n");
		printf("                or a name relative to the show folder. Without it the\n");
		printf("                player follows the schedules in mini_light_player.json\n");
		printf("                (or scottplayer.json, for an existing show)\n");
		printf("  media_file    optional audio file overriding the one named in the fseq\n");
		printf("  --multisync   act as an FPP multisync master\n");
	}
}

int main(int argc, char *argv[])
{
	char const* const exe = argc > 0 ? argv[0] : "mini_light_player";

	bool multisync{ false };
	std::vector<std::string> positional;

	for (int i = 1; i < argc; ++i)
	{
		std::string const arg = argv[i];
		if (arg == "--multisync")
		{
			multisync = true;
		}
		else if (arg == "--help" || arg == "-h")
		{
			PrintUsage(exe);
			return 0;
		}
		else if (arg.rfind("--", 0) == 0)
		{
			printf("Unknown option: %s\n\n", arg.c_str());
			PrintUsage(exe);
			return 1;
		}
		else
		{
			positional.push_back(arg);
		}
	}

	if (positional.empty())
	{
		PrintUsage(exe);
		return 1;
	}

	try
	{
		MiniPlayer player(positional[0]);
		player.SetMultisync(multisync);

		if (positional.size() >= 2)
		{
			player.PlayOnce(positional[1], positional.size() >= 3 ? positional[2] : std::string());
		}

		player.Run();
	}
	catch (std::exception const& ex)
	{
		printf("MiniPlayer error: %s\n", ex.what());
		return 1;
	}
	catch (...)
	{
		printf("MiniPlayer error: unknown exception\n");
		return 1;
	}
	return 0;
}
