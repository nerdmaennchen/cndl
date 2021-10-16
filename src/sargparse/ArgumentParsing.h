#pragma once

#include <string>
#include <regex>
#include <vector>

#include "Parameter.h"

namespace sargp {

/**
 * parse the arguments and set all parameters with the parsed values
 */
void parseArguments(int argc, char const* const* argv);
/**
 * parse the arguments but only set the parameters specified in the set to the parsed values
 */
void parseArguments(int argc, char const* const* argv, std::vector<ParameterBase*> const& targetParameters);

std::string generateHelpString(std::regex const& filter=std::regex{".*"});
std::string generateGroffString(std::string const& program_name, std::string const& description, Command const& command=Command::getDefaultCommand());

std::string compgen(int argc, char const* const* argv);

std::vector<Command*> getActiveCommands();

void callCommands();

}
