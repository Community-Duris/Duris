// Offline editor contract validator. Gameplay availability/revision checks run
// in the server after spell pointers and prototypes have been loaded.
#include "item/studio_ability_model.h"
#include <fstream>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "usage: studio-ability-validator catalog.json\n";
		return 2;
	}
	std::ifstream file(argv[1], std::ios::binary);
	if (!file)
	{
		std::cerr << "cannot read catalog\n";
		return 2;
	}
	std::string text(1024 * 1024 + 1, '\0');
	file.read(text.data(), text.size());
	text.resize(static_cast<size_t>(file.gcount()));
	if (file.bad())
		return 2;
	studio_ability_catalog catalog;
	std::string error;
	if (!parse_studio_ability_catalog(text, catalog, error))
	{
		std::cerr << error << '\n';
		return 1;
	}
	std::cout << "Valid schemaVersion 1 catalog: " << catalog.size() << " abilities\n";
}
