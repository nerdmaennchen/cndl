#include <iostream>

#include "sargparse/Parameter.h"
#include "sargparse/ArgumentParsing.h"


namespace
{

sargp::Parameter<std::optional<std::string>> printHelp{{}, "help", "print this help add a string which will be used in a grep-like search through the parameters"};

}

int main(int argc, char** argv) {
	if (std::string(argv[argc-1]) == "--compgen") {
		std::cout << sargp::compgen(argc-2, argv+1);
		return 0;
	}
    
	// pass everything except the name of the application
	sargp::parseArguments(argc-1, argv+1);

	if (printHelp) {
		std::cout << sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"});
		return 0;
	}

	sargp::callCommands();

    return 0;
}