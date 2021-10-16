#include "Parameter.h"

#include <algorithm>

namespace sargp {

TaskBase::TaskBase(Command& c) : _command{c} {
	_command.registerTask(*this);
}
TaskBase::~TaskBase() {
	_command.deregisterTask(*this);
}

ParameterBase::ParameterBase(std::string const& argName, DescribeFunc const& describeFunc, Callback cb, ValueHintFunc const& hintFunc, Command& command, std::type_info const& _type_info)
	: _argName{argName}
	, _describeFunc{describeFunc}
	, _cb{cb}
	, _hintFunc{hintFunc}
	, _command{command}
	, type_info{_type_info}
{
	_command.registerParameter(*this);
}

ParameterBase::~ParameterBase() {
	_command.deregisterParameter(*this);
}


Command& Command::getDefaultCommand() {
	static Command instance{Command::DefaultCommand{}};
	return instance;
}

Command::Command(DefaultCommand)
{}

Command& getDefaultCommand() {
	return Command::getDefaultCommand();
}

auto Command::findSubCommand(std::string const& subcommand) const -> Command const* {
	for (auto sub : subcommands) {
		if (sub->getName() == subcommand) {
			return sub;
		}
	}
	return nullptr;
}

auto Command::findSubCommand(std::string const& subcommand) -> Command* {
	for (auto sub : subcommands) {
		if (sub->getName() == subcommand) {
			return sub;
		}
	}
	return nullptr;
}

auto Command::findParameter(std::string const& parameter) const -> ParameterBase const* {
	for (auto p : parameters) {
		if (p->getArgName() == parameter) {
			return p;
		}
	}
	return nullptr;
}

auto Command::findParameter(std::string const& parameter) -> ParameterBase* {
	for (auto p : parameters) {
		if (p->getArgName() == parameter) {
			return p;
		}
	}
	return nullptr;
}




}
