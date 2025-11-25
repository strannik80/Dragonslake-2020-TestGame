#pragma once
#include <string>

extern int windowWidth, windowHeight;
extern int mapWidth, mapHeight;
extern int numEnemies, numAmmo;

namespace TestGame
{
	void ParseCLArguments(int argc, char* argv[])
	{
		if (argc > 1)
		{
			for (int i = 1; i < argc; i++)
			{
				std::string arg = argv[i];

				if (arg == "-window")
				{
					arg = argv[++i]; 

					size_t xPos = 0; 
					for (int j = 0; j < arg.length(); j++)
					{
						if ((arg.at(j) == 'x') || (arg.at(j) == 'X'))
							xPos = j;
					}

					std::string temp = arg.substr(0, xPos);
					windowWidth = stoi(temp);
					temp = arg.substr(xPos + 1, arg.length() - (xPos + 1));
					windowHeight = stoi(temp);					
				}

				if (arg == "-map")
				{
					arg = argv[++i];

					size_t xPos = 0;
					for (int j = 0; j < arg.length(); j++)
					{
						if ((arg.at(j) == 'x') || (arg.at(j) == 'X'))
							xPos = j;
					}

					std::string temp = arg.substr(0, xPos);
					mapWidth = stoi(temp);
					temp = arg.substr(xPos + 1, arg.length() - (xPos + 1));
					mapHeight = stoi(temp);
				}

				if (arg == "-num_enemies")
				{
					std::string temp = argv[++i];
					numEnemies = stoi(temp);
				}

				if (arg == "-num_ammo")
				{
					std::string temp = argv[++i];
					numAmmo = stoi(temp);
				}
			}
		}
	}
}
