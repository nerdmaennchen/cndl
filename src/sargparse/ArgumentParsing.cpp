#include "ArgumentParsing.h"
#include "File.h"
#include <algorithm>

namespace sargp {

namespace {

bool isArgName(std::string const& str) {
	return str.find("--", 0) == 0 and str.size() > 2;
}

template<typename ParamCallback>
int tokenizeArgument(int argc, char const* const* argv, std::string const& argName, bool& parseEverything, ParamCallback&& paramCB) {
	std::vector<std::string> arguments;
	int idx{0};
	for (; idx < argc; ++idx) {
		std::string v{argv[idx]};
		if (not parseEverything and v == "--") {
			parseEverything = true;
			continue;
		} else if (isArgName(v) and not parseEverything) {
			break;
		}
		arguments.emplace_back(std::move(v));
	}
	return paramCB(argName, arguments);
}

template<typename CommandCallback, typename ParamCallback>
bool tokenize(int argc, char const* const* argv, CommandCallback&& commandCB, ParamCallback&& paramCB) {
	int idx{0};
	bool hasTrailingParameters{false};
	Command* lastCommand = nullptr;
	bool parseEverything{false};
	while (idx < argc) {
		auto curArgName = std::string{argv[idx]};

		// Parse for complete trailing capture
		if (curArgName == "--") {
			++idx;
			parseEverything = true;
			idx += tokenizeArgument(argc-idx, argv+idx, "", parseEverything, paramCB);
		// parsing arguments
		} else if (isArgName(curArgName)) {
			++idx;
			auto argName  = curArgName.substr(2);
			idx += tokenizeArgument(argc-idx, argv+idx, argName, parseEverything, paramCB);
		// parsing partial trailing capture
		} else if (hasTrailingParameters and lastCommand->findSubCommand(argv[idx]) == nullptr) {
			idx += tokenizeArgument(argc-idx, argv+idx, "", parseEverything, paramCB);
		// parse for commands
		} else {
			++idx;
			lastCommand = commandCB(curArgName);
			hasTrailingParameters = lastCommand and lastCommand->findParameter("");
		}
		if (parseEverything) {
			return true;
		}

	}
	return false;
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
		auto subCommand = argProviders.back()->findSubCommand(commandName);
		if (subCommand == nullptr) {
			throw std::invalid_argument("command '" + commandName + "' is not implemented");
		}
		subCommand->setActive(true);
		argProviders.push_back(subCommand);
		return subCommand;
	};

	auto paramCB = [&](std::string const& argName, std::vector<std::string> const& arguments) {
		for (auto iter = rbegin(argProviders); iter != rend(argProviders); ++iter) {
			auto argProvider = *iter;
			auto param = argProvider->findParameter(argName);

			if (param) try {
				int amount = param->parse(arguments);
				param->parsed();
				return amount;
			} catch (sargp::parsing::detail::ParseError const& error) {
				throw std::invalid_argument("cannot parse arguments for \"" + argName + "\" - " + error.what());
			}
		}
		throw std::invalid_argument("argument " + argName + " is not implemented");
	};

	tokenize(argc, argv, commandCB, paramCB);
}

void parseArguments(int argc, char const* const* argv, std::vector<ParameterBase*> const& targetParameters) {
	tokenize(argc, argv, [](std::string const&){ return nullptr;}, [&](std::string const& argName, std::vector<std::string> const& arguments)
	{
		auto target = std::find_if(targetParameters.begin(), targetParameters.end(), [&](ParameterBase*p){ return p->getArgName() == argName; });
		if (target == targetParameters.end()) {
			return 0;
		}
		try {
			return (*target)->parse(arguments);
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
		int maxArgNameLen = 10;
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
				if (command->findParameter("")) {
					helpString += command->getName() + " <arguments>\n";
				} else {
					helpString += command->getName() + "\n";
				}
				helpString += "    " + command->getDescription() + "\n";
				helpString += "\nparameters for command " + command->getName() + "\n\n";
			}

			for (auto const& param : command->getParameters()) {
				std::string name = param->getArgName();
				if (name.empty()) {
					name = "<arguments>";
				} else {
					name = "--" + name;
				}
				if (std::regex_match(name, filter)) {
					helpString += name + std::string(maxArgNameLen - name.size(), ' ');
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
	for (auto iter = rbegin(active_commands); iter != rend(active_commands); ++iter) {
		Command* command = *iter;
		helpString += helpStrForCommand(command);
	}

	return helpString;
}

std::string compgen(int argc, char const* const* argv) {
	std::vector<Command*> argProviders = {&Command::getDefaultCommand()};
	std::string lastArgName;
	std::vector<std::string> lastArguments;

	auto commandCB = [&](std::string const& commandName) -> Command* {
		lastArgName = "";
		lastArguments.clear();
		auto subCommand = argProviders.back()->findSubCommand(commandName);
		if (subCommand) {
			argProviders.push_back(subCommand);
			subCommand->setActive(true);
			return subCommand;
		}
		return nullptr;
	};

	auto paramCB = [&](std::string const& argName, std::vector<std::string> const& arguments) {
		lastArgName = argName;
		lastArguments = arguments;
		for (auto iter = rbegin(argProviders); iter != rend(argProviders); ++iter) {
			auto argProvider = *iter;
			auto param = argProvider->findParameter(argName);
			if (param) try {
				return param->parse(arguments);
			} catch (sargp::parsing::detail::ParseError const&) {
			} catch (std::invalid_argument const&) {
			}
		}
		return 0;
	};

	bool onlyTrailingArguments = tokenize(argc-1, argv, commandCB, paramCB); // user has given "-- "
	lastArguments.push_back(argv[argc-1]);

	std::set<std::string> hints;

	bool canAcceptNextArg = true;
	if (onlyTrailingArguments) {
		auto param = argProviders.back()->findParameter("");
		if (param) {
			auto [cur_canAcceptNextArg, cur_hints] = param->getValueHints(lastArguments);
			canAcceptNextArg = false;
			hints.insert(cur_hints.begin(), cur_hints.end());
		}
	}

	if (canAcceptNextArg) {
		for (auto iter = rbegin(argProviders); iter != rend(argProviders); ++iter) {
			auto argProvider = *iter;
			auto param = argProvider->findParameter(lastArgName);
			if (param and (lastArguments.back().find("-", 0) != 0)) {
				auto [cur_canAcceptNextArg, cur_hints] = param->getValueHints(lastArguments);
				canAcceptNextArg &= cur_canAcceptNextArg;
				hints.insert(cur_hints.begin(), cur_hints.end());
				break;
			}
		}
	}

	if (canAcceptNextArg) {
		auto const& subC = argProviders.back()->getSubCommands();
		for (Command const* c : subC) {
			hints.emplace(c->getName());
		}

		for (auto argProvider : argProviders) {
			for (auto const* p : argProvider->getParameters()) {
				if (p->getArgName() != "") {
					hints.insert("--" + p->getArgName());
				}
			}
		}
	}
	std::string compgen_str;
	if (not hints.empty()) {
		compgen_str += std::accumulate(next(begin(hints)), end(hints), *begin(hints), [](std::string const& l , std::string const& r){
			return l + "\n" + r;
		});
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
