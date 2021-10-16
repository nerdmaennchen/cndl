#pragma once

#include <map>
#include <set>
#include <vector>
#include <functional>
#include <numeric>
#include <initializer_list>
#include <algorithm>
#include <memory>

#include "ParameterParsing.h"

namespace sargp {

struct Command;
Command& getDefaultCommand();

struct ParameterBase {
	using Callback     = std::function<void()>;
	using DescribeFunc = std::function<std::string()>;
	/**
	 *  given some strings that might be passed as arguments to the Parameter
	 *  can the amount of arguments be accepted (that is in terms of bash completion can other arguments be started)
	 *  and what values can be proposed to the user
	 */
	using ValueHintFunc = std::function<std::pair<bool, std::set<std::string>>(std::vector<std::string> const&)>;
	virtual ~ParameterBase();

	virtual int parse([[maybe_unused]] std::vector<std::string> const&  args) = 0;
	virtual std::string stringifyValue() const = 0;

	virtual std::string describe() const {
		return _describeFunc();
	}

	virtual auto getValueHints(std::vector<std::string> const& args) const -> std::pair<bool, std::set<std::string>> {
		if (_hintFunc) {
			return _hintFunc(args);
		}
		return std::make_pair<bool, std::set<std::string>>(args.size() > 1, {});
	}

	auto getArgName() const -> std::string const& {
		return _argName;
	}

	/**
	 * return true iff the Parameter was set (by any means)
	 * even if the contained value is the default value
	 */
	operator bool() const { return _valSpecified; };

	// called after a successful parse
	void parsed() {
		_valSpecified = true;
		if (_cb) {
			_cb();
		}
	}

	std::type_info const& get_type() const {
		return type_info;
	}
protected:
	ParameterBase(std::string const& argName, DescribeFunc const& describeFunc, Callback cb, ValueHintFunc const& hintFunc, Command& command, std::type_info const& type_info);

	std::string _argName;
	DescribeFunc _describeFunc;
	Callback _cb;
	ValueHintFunc _hintFunc;
	Command& _command;
	bool _valSpecified {false};
	std::type_info const& type_info;
};

template<typename T>
struct TypedParameter : ParameterBase {
protected:
	using SuperClass = ParameterBase;
	using value_type = T;

	value_type _val;
public:
	TypedParameter(T const& defaultVal, std::string const& argName, std::string const& description, Callback cb=Callback{}, ValueHintFunc hintFunc=ValueHintFunc{}, Command& command=getDefaultCommand())
	: TypedParameter(defaultVal, argName, [=]{return description;}, cb, hintFunc, command)
	{}
	TypedParameter(T const& defaultVal, std::string const& argName, DescribeFunc const& description, Callback cb=Callback{}, ValueHintFunc hintFunc=ValueHintFunc{}, Command& command=getDefaultCommand());

	int parse(std::vector<std::string> const& args) override {
		int amount;
		std::tie(_val, amount) = parsing::parse<T>(args);
		return amount;
	}

	T& operator *() {
		return _val;
	}
	T const& operator *() const {
		return _val;
	}
	T* operator->() {
		return &_val;
	}
	T const* operator->() const {
		return &_val;
	}

	T const& get() const {
		return _val;
	}

	T& get() {
		return _val;
	}
};

// an intermediate type broker to inject type specific specializations eg. for rendering
template<typename T>
struct SpeciallyTypedParameter : TypedParameter<T> {
	using TypedParameter<T>::TypedParameter;
};

template<>
struct SpeciallyTypedParameter<bool> : TypedParameter<bool> {
	using SuperClass = TypedParameter<bool>;
	using value_type = SuperClass::value_type;
	using SuperClass::SuperClass;

	auto getValueHints(std::vector<std::string> const& args) const -> std::pair<bool, std::set<std::string>> override {
		if (SuperClass::_hintFunc) {
			return SuperClass::_hintFunc(args);
		}
		if (args.empty() or (args.size() == 1 and args[0] != "true" and args[0] != "false")) {
			return std::make_pair<bool, std::set<std::string>>(true, {"true", "false"});
		}
		return std::make_pair<bool, std::set<std::string>>(true, {});
	}
};

template<typename T>
struct SpeciallyTypedParameter<std::optional<T>> : TypedParameter<std::optional<T>> {
	using SuperClass = TypedParameter<std::optional<T>>;
	using value_type = typename SuperClass::value_type;
	using SuperClass::SuperClass;

	auto getValueHints(std::vector<std::string> const& args) const -> std::pair<bool, std::set<std::string>> override {
		if (SuperClass::_hintFunc) {
			return SuperClass::_hintFunc(args);
		}
		return std::make_pair<bool, std::set<std::string>>(true, {});
	}
};

template<typename T>
struct SpeciallyTypedParameter<std::vector<T>> : TypedParameter<std::vector<T>> {
	using SuperClass = TypedParameter<std::vector<T>>;
	using value_type = typename SuperClass::value_type;
	using SuperClass::SuperClass;

	auto getValueHints(std::vector<std::string> const& args) const -> std::pair<bool, std::set<std::string>> override {
		if (SuperClass::_hintFunc) {
			return SuperClass::_hintFunc(args);
		}
		return std::make_pair<bool, std::set<std::string>>(true, {});
	}
};
template<typename T>
struct SpeciallyTypedParameter<std::set<T>> : TypedParameter<std::set<T>> {
	using SuperClass = TypedParameter<std::set<T>>;
	using value_type = typename SuperClass::value_type;
	using SuperClass::SuperClass;

	auto getValueHints(std::vector<std::string> const& args) const -> std::pair<bool, std::set<std::string>> override {
		if (SuperClass::_hintFunc) {
			return SuperClass::_hintFunc(args);
		}
		return std::make_pair<bool, std::set<std::string>>(true, {});
	}
};

template<typename T>
struct Parameter : SpeciallyTypedParameter<T> {
	using SuperClass = SpeciallyTypedParameter<T>;
	using SuperClass::SuperClass;

	std::string stringifyValue() const override {
		return parsing::stringify(SuperClass::_val);
	}
};

struct Flag final : Parameter<bool> {
	using SuperClass = Parameter<bool>;
	using Callback = typename SuperClass::Callback;
	Flag(std::string const& argName, std::string const& description, Callback cb=Callback{}, Command& command=getDefaultCommand())
	: Parameter<bool>(false, argName, description, cb, ValueHintFunc{}, command)
	{}

	std::string stringifyValue() const override {
		return "";
	}
};


template<typename T>
struct Choice final : TypedParameter<T> {
	using SuperClass = TypedParameter<T>;
	using Callback   = typename SuperClass::Callback;
private:
	std::map<std::string, T> _name2ValMap;
public:
	Choice(T const& defaultVal, std::string const& argName, std::map<std::string, T> const& namedValues, std::string const& description, Callback cb=Callback{}, Command& command=getDefaultCommand())
	: SuperClass(defaultVal, argName, description, cb, {}, command)
	, _name2ValMap(namedValues)
	{}

	int parse(std::vector<std::string> const& args) override {
		if (args.empty()) {
			throw parsing::detail::ParseError("a choice must be given exactly one value");
		}
		auto it = _name2ValMap.find(args[0]);
		if (it == _name2ValMap.end()) {
			throw parsing::detail::ParseError("cannot interpret " + args[0] + " as a valid value for " + SuperClass::getArgName());
		}
		SuperClass::_val = it->second;
		return 1;
	}

	std::string stringifyValue() const override {
		for (auto const& n2v : _name2ValMap) {
			if (n2v.second == SuperClass::_val) {
				return n2v.first;
			}
		}
		return "";
	}

	auto getValueHints(std::vector<std::string> const& args) const -> std::pair<bool, std::set<std::string>> override {
		if (SuperClass::_hintFunc) {
			return SuperClass::_hintFunc(args);
		}
		if (args.size() != 1) {
			return {true, {}};
		}
		std::set<std::string> names;
		for (auto const& n2v : _name2ValMap) { names.emplace(n2v.first); };
		return std::make_pair<bool, std::set<std::string>>(false , std::move(names));
	}

	std::string describe() const override {
		std::string description = "valid values: {";
		if (not _name2ValMap.empty()) {
			description += std::accumulate(std::next(_name2ValMap.begin()), _name2ValMap.end(), _name2ValMap.begin()->first, [](std::string const& left, std::pair<std::string, T> const& p) {
				return left + " " + p.first;
			});
		}
		description += "}\n\t";
		description += SuperClass::describe();
		return description;
	}
};

struct Section final {
private:
	std::string _name;

public:
	Section(std::string const& name)
	: _name(name + ".")
	{}

	template<typename T, typename... Args>
	[[nodiscard]] auto Parameter(T const& defaultVal, std::string const& argName, Args &&... args) const -> Parameter<T> {
		return ::sargp::Parameter<T>{defaultVal, _name + argName, std::forward<Args>(args)...};
	}
	template<typename... Args>
	[[nodiscard]] auto Flag(std::string const& argName, Args &&... args) const -> Flag {
		return ::sargp::Flag{_name + argName, std::forward<Args>(args)...};
	}
	template<typename T, typename... Args>
	[[nodiscard]] auto Choice(T const& defaultVal, std::string const& argName, std::map<std::string, T> const& namedValues, Args &&... args) const -> Choice<T> {
		return ::sargp::Choice<T>{defaultVal, _name + argName, namedValues, std::forward<Args>(args)...};
	}
};

struct TaskBase {
	TaskBase(Command& c);
	virtual ~TaskBase();
	virtual void operator()() = 0;

protected:
	Command& _command;
};

struct Command final {
	using DescribeFunc  = ParameterBase::DescribeFunc;
	using ValueHintFunc = ParameterBase::ValueHintFunc;
	using Callback      = ParameterBase::Callback;

private:
	std::string               _name;
	std::string               _description;
	std::vector<TaskBase*>    _tasks;
	std::unique_ptr<TaskBase> _defaultTask;
	bool                      _isActive {false};

	std::vector<ParameterBase*> parameters;
	std::vector<Command*>       subcommands;
	Command* _parentCommand {nullptr};

	template<typename CB>
	Command(Command* parentCommand, std::string const& name, std::string const& description, CB&& cb);

	struct DefaultCommand {};
	Command(DefaultCommand);
public:
	Command(std::string const& name, std::string const& description)
		: Command{nullptr, name, description, []{}} {}

	template<typename CB>
	Command(std::string const& name, std::string const& description, CB&& cb)
		: Command(nullptr, name, description, cb) {}

	~Command() {
		if (not _parentCommand) {
			return;
		}
		auto& pSubC = _parentCommand->subcommands;
		pSubC.erase(std::remove(begin(pSubC), end(pSubC), this), end(pSubC));
	}

	auto getName() const -> decltype(_name) const& {
		return _name;
	}

	auto getDescription() const -> decltype(_description) const& {
		return _description;
	}
	auto getSubCommands() const -> decltype(subcommands) const& {
		return subcommands;
	}

	auto findSubCommand(std::string const& subcommand) const -> Command const*;
	auto findSubCommand(std::string const& subcommand) -> Command*;

	auto getParentCommand() const -> decltype(_parentCommand) {
		return _parentCommand;
	}

	void registerParameter(ParameterBase& parameter) {
		parameters.emplace_back(&parameter);
	}
	void deregisterParameter(ParameterBase& parameter) {
		parameters.erase(std::remove(begin(parameters), end(parameters), &parameter), end(parameters));
	}

	void registerTask(TaskBase& task) {
		_tasks.emplace_back(&task);
	}
	void deregisterTask(TaskBase& task) {
		_tasks.erase(std::remove(begin(_tasks), end(_tasks), &task), end(_tasks));
	}

	auto getParameters() const -> decltype(parameters) const& {
		return parameters;
	}
	auto findParameter(std::string const& parameter) const -> ParameterBase const*;
	auto findParameter(std::string const& parameter) -> ParameterBase*;

	void setActive(bool active) {
		_isActive = active;
	}

	operator bool() const {
		return _isActive;
	}

	void callCBs() {
		for (auto task : _tasks) {
			(*task)();
		}
	}

	static Command& getDefaultCommand();

	// parameters that are enabled if this Command is active
	template<typename T>
	[[nodiscard]] auto Parameter(T const& defaultVal, std::string const& argName, std::string const& description, Callback cb={}, ValueHintFunc hintFunc=ValueHintFunc{}) -> ::sargp::Parameter<T> {
		return ::sargp::Parameter<T>{defaultVal, argName, description, cb, hintFunc, *this};
	}
	template<typename T>
	[[nodiscard]] auto Parameter(T const& defaultVal, std::string const& argName, DescribeFunc const& description, Callback cb={}, ValueHintFunc hintFunc=ValueHintFunc{}) -> ::sargp::Parameter<T> {
		return ::sargp::Parameter<T>{defaultVal, argName, description, cb, hintFunc, *this};
	}

	[[nodiscard]] auto Flag(std::string const& argName, std::string const& description, Callback cb={}) -> ::sargp::Flag {
		return ::sargp::Flag{argName, description, cb, *this};
	}
	template<typename T, typename... Args>
	[[nodiscard]] auto Choice(T const& defaultVal, std::string const& argName, std::map<std::string, T> const& namedValues, std::string const& description, Callback cb=Callback{}) -> ::sargp::Choice<T> {
		return ::sargp::Choice<T>{defaultVal, argName, namedValues, description, cb, *this};
	}
	[[nodiscard]] auto Section(std::string const& name) -> Section {
		return ::sargp::Section(name);
	}

	template<typename CB>
	[[nodiscard]] auto SubCommand(std::string const& name, std::string const& description, CB&& cb) -> Command {
		return ::sargp::Command(this, name, description, std::forward<CB>(cb));
	}
};

template<typename CB>
struct Task final : TaskBase {
	Task(CB&& cb, Command& command = Command::getDefaultCommand())
	: TaskBase{command}, _cb{std::forward<CB>(cb)} {}
	virtual ~Task() = default;

	void operator()() override {
		return _cb();
	}
private:
	CB _cb;
};
template<typename CB> Task(CB) -> Task<CB>;

template<typename CB>
Command::Command(Command* parentCommand, std::string const& name, std::string const& description, CB&& cb) 
	: _name(name)
	, _description(description)
	, _tasks{}
	, _defaultTask{std::make_unique<Task<CB>>(std::forward<CB>(cb), *this)}
	, _parentCommand{parentCommand?nullptr:&Command::getDefaultCommand()}
{
	_parentCommand->subcommands.emplace_back(this);
}

template<typename T>
TypedParameter<T>::TypedParameter(T const& defaultVal, std::string const& argName, DescribeFunc const& description, Callback cb, ValueHintFunc hintFunc, Command& command)
	: SuperClass(argName, description, cb, hintFunc, command, typeid(T))
	, _val{defaultVal}
{}

}
