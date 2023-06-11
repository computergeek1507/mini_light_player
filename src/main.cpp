#include "miniplayer.h"

int main(int argc, char *argv[])
{    
	try
	{
#ifdef _DEBUG
		if (argc == 2)
		{
			printf("Arg: %s\n", argv[1]);
			MiniPlayer main(argv[1]);		
		}
#else
		if (argc == 1)
		{
			printf("Arg: %s\n", argv[0]);
			MiniPlayer main(argv[0]);			
		}
#endif
		else
		{
			printf("need to pass in a show folder");
		}
	}
	catch( std::exception &ex )
	{
		printf("MiniPlayer Error");
		printf(ex.what());
	}
	catch(...)
	{
		printf("MiniPlayer Error");
	}
	return {0};
}
