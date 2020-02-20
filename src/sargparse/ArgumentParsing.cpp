#include "ArgumentParsing.h"
#include "File.h"
#include <algorithm>

namespace sargp
{

namespace
{

bool isArgName(std::string const& str) {
	return str.find("--", 0) == 0;
}

template<typename CommandCallback, typename ParamCallback>
void tokenize(int argc, char const* const* argv, CommandCallback&& commandCB, ParamCallback&& paramCB) {
	int idx{0};
	for (; idx < argc; ++idx) {
		std::string curArgName = std::string{argv[idx]};
		if (isArgName(curArgName)) {
			break;
		} else {
			commandCB(curArgName);
		}
	}
	while (idx < argc) {
		// idx is the index of the last argName that was encountered
		std::string argName = std::string{argv[idx]+2}; // +2 to remove the double dash
		std::vector<std::string> arguments;
		++idx;
		for (; idx < argc; ++idx) {
			std::string v{argv[idx]};
			if (isArgName(v)) {
				break;
			}
			arguments.emplace_back(std::move(v));
		}
		paramCB(argName, arguments);
	}
}

}

std::vector<Command*> getActiveCommands() {
    std::vector<Command*> activeCommands = {&Command::getDefaultCommand()};
    while (true) {
        auto& subC = activeCommands.back()->getSubCommands();
        auto it = std::find_if(begin(subC), end(subC), [](Command const* p){
            return static_cast<bool>(*p);
        });
        if (it == subC.end()) {
            break;
        }
        activeCommands.emplace_back(*it);
    }
    return activeCommands;
}

void parseArguments(int argc, char const* const* argv) {
	std::vector<Command*> argProviders = {&Command::getDefaultCommand()};

    auto commandCB = [&](std::string const& commandName) {
        auto const& subC = argProviders.back()->getSubCommands();
		auto target = std::find_if(begin(subC), end(subC), [&](Command const* cmd){
            return cmd->getName() == commandName;
        });
		if (target == subC.end()) {
			throw std::invalid_argument("command " + commandName + " is not implemented");
		}
        (*target)->setActive(true);
        argProviders.push_back(*target);
	};

    auto paramCB = [&](std::string const& argName, std::vector<std::string> const& arguments) {
		bool found = false;
		for (auto argProvider : argProviders) {
            auto& params = argProvider->getParameters();
			auto target = std::find_if(begin(params), end(params), [&](ParameterBase const* p){
                return p->getArgName() == argName;
            });
			if (target == params.end()) {
				continue;
			}
			found = true;
            try {
                (*target)->parse(arguments);
                (*target)->parsed();
            } catch (sargp::parsing::detail::ParseError const& error) {
                throw std::invalid_argument("cannot parse arguments for \"" + argName + "\" - " + error.what());
            }
		}
		if (not found) {
			throw std::invalid_argument("argument " + argName + " is not implemented");
		}
	};

	tokenize(argc, argv, commandCB, paramCB);
}

void parseArguments(int argc, char const* const* argv, std::vector<ParameterBase*> const& targetParameters) {
	tokenize(argc, argv, [](std::string const&){}, [&](std::string const& argName, std::vector<std::string> const& arguments)
	{
		auto target = std::find_if(targetParameters.begin(), targetParameters.end(), [&](ParameterBase*p){ return p->getArgName() == argName; });
		if (target == targetParameters.end()) {
			return;
		}
		try {
			(*target)->parse(arguments);
		} catch (sargp::parsing::detail::ParseError const& error) {
			throw std::invalid_argument("cannot parse arguments for \"" + argName + "\"");
		}
	});
}

std::string generateHelpString(std::regex const& filter) {
	std::string helpString;

	auto active_commands = getActiveCommands();

    auto const& subCommands = active_commands.back()->getSubCommands();
	if (not subCommands.empty()) { // if there is at least one subcommand
		helpString += "valid subcommands:\n\n";
		int maxCommandStrLen = (*std::max_element(begin(subCommands), end(subCommands), 
            [](Command const* a, Command const* b) {
                return a->getName().size() < b->getName().size(); 
            }))->getName().size();
		maxCommandStrLen += 2;// +2 cause we print two spaces at the beginning
		for (auto const& subC : subCommands) {
            std::string name = subC->getName();
            helpString += "  " + name + std::string(maxCommandStrLen - name.size()+1, ' ') + subC->getDescription() + "\n";
		}
		helpString += "\n";
	}
    auto helpStrForCommand = [&](Command const* command) {
        std::string helpString;
		int maxArgNameLen = 0;
		bool anyMatch = false;
        for (auto const& param : command->getParameters()) {
            if (std::regex_match(param->getArgName(), filter)) {
                anyMatch = true;
                maxArgNameLen = std::max(maxArgNameLen, static_cast<int>(param->getArgName().size()));
            }
        }
		if (anyMatch) {
			maxArgNameLen += 4;
			if (command->getName().empty()) {
				helpString += "\nglobal parameters:\n\n";
			} else {
				helpString += "\nparameters for command " + command->getName() + ":\n\n";
			}

            for (auto const& param : command->getParameters()) {
                std::string const& name = param->getArgName();
                if (std::regex_match(param->getArgName(), filter)) {
                    helpString += "--" + name + std::string(maxArgNameLen - name.size(), ' ');
                    if (not *param) {// the difference are the brackets!
                        helpString += "(" + param->stringifyValue() + ")";
                    } else {
                        helpString += param->stringifyValue();
                    }
                    helpString += "\n";
                    auto&& description = param->describe();
                    if (not description.empty()) {
                        helpString += "    " + description + "\n";
                    }
                }
            }
		}
        return helpString;
    };
    for (Command* command : active_commands) {
        helpString += helpStrForCommand(command);
	}

	return helpString;
}

std::string compgen(int argc, char const* const* argv) {
	std::vector<Command*> argProviders = {&Command::getDefaultCommand()};
	std::string lastArgName;
	std::vector<std::string> lastArguments;

    auto commandCB = [&](std::string const& commandName) {
        auto const& subC = argProviders.back()->getSubCommands();
		auto target = std::find_if(begin(subC), end(subC), [&](Command const* cmd){
            return cmd->getName() == commandName;
        });
		if (target != subC.end()) {
            argProviders.push_back(*target);
            (*target)->setActive(true);
		}
	};

    auto paramCB = [&](std::string const& argName, std::vector<std::string> const& arguments) {
        lastArgName = argName;
        lastArguments = arguments;
		for (auto argProvider : argProviders) {
            auto& params = argProvider->getParameters();
			auto target = std::find_if(begin(params), end(params), [&](ParameterBase const* p){
                return p->getArgName() == argName;
            });
			if (target == params.end()) {
				continue;
			}
            try {
                (*target)->parse(arguments);
            } catch (sargp::parsing::detail::ParseError const&) {
            } catch (std::invalid_argument const&) {
            }
		}
	};

	tokenize(argc, argv, commandCB, paramCB);

	std::set<std::string> hints;
    bool can_accept_file {false};
    bool can_accept_directory {false};

	if (lastArgName.empty()) {
        auto const& subC = argProviders.back()->getSubCommands();
		for (Command const* c : subC) {
			hints.emplace(c->getName());
		}
	}

	bool canAcceptNextArg = true;
	for (auto argProvider : argProviders) {
        auto const& params = argProvider->getParameters();
        auto target = std::find_if(begin(params), end(params), [&](ParameterBase const* p) {
            return p->getArgName() == lastArgName;
        });
        if (target == params.end()) {
            continue;
        }
        auto useParam = [&] {
            auto [cur_canAcceptNextArg, cur_hints] = (*target)->getValueHints(lastArguments);
            canAcceptNextArg &= cur_canAcceptNextArg;
            hints.insert(cur_hints.begin(), cur_hints.end());
        };
        if ((*target)->get_type() == typeid(File)) {
            useParam();
            can_accept_file = not canAcceptNextArg;
        } else if ((*target)->get_type() == typeid(Directory)) {
            useParam();
            can_accept_directory = not canAcceptNextArg;
        } else {
            useParam();
        }
	}

	if (canAcceptNextArg) {
		for (auto argProvider : argProviders) {
			for (auto const* p : argProvider->getParameters()) {
				if (not *p) {
					hints.insert("--" + p->getArgName());
				}
			}
		}
	}
    std::string compgen_str;
    if (can_accept_directory) {
        compgen_str += " -d ";
    }
    if (can_accept_file) {
        compgen_str += " -f ";
    }
    if (not hints.empty()) {
        compgen_str += "-W ' ";
        compgen_str += std::accumulate(next(begin(hints)), end(hints), *begin(hints), [](std::string const& l , std::string const& r){
            return l + " " + r;
        });
        compgen_str += " '";
    }
	return compgen_str;
}

void callCommands() {
	auto const& commands = getActiveCommands();
	for (Command* cmd : commands) {
		cmd->callCBs();
	}
}

}
