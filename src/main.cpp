#include "miniplayer.h"

#include <cstdio>
#include <string>

namespace
{
	void PrintUsage(char const* exe)
	{
		printf("mini_light_player - fseq show player\n\n");
		printf("Usage: %s <show_folder> [sequence.fseq] [media_file]\n\n", exe);
		printf("  show_folder   xLights show directory holding xlights_networks.xml\n");
		printf("  sequence.fseq optional sequence to play immediately; may be a path\n");
		printf("                or a name relative to the show folder\n");
		printf("  media_file    optional audio file overriding the one named in the fseq\n");
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		PrintUsage(argc > 0 ? argv[0] : "mini_light_player");
		return 1;
	}

	try
	{
		MiniPlayer player(argv[1]);

		if (argc >= 3)
		{
			player.Play(argv[2], argc >= 4 ? argv[3] : std::string());
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
