# sargparse
A very easy to use argument parser

sargparse is intended to be used in very modular software where parameters can reside all over the source files

Here is a small yet extensive example about its usage:

File foo.cpp:
~~~
#include <sargparse/Parameter.h>
#include <iostream>

namespace {
// all parameters comming from this section have "mySection." as their name prefix
auto mySection = sargp::Section{"mySection"};
// here are some demonstrations of how parameters can be registered
// the arguments passed to Parameters are pretty easy: default_value, argument_name, description_for_help_text
auto myIntParam    = mySection.Parameter<int>(123, "integer", "an integer argument");
auto myDoubleParam = mySection.Parameter<double>(M_PI, "double", "a double argument");
auto myStringParam = mySection.Parameter<std::string>("some string value", "string", "a string argument");
auto myFlag        = mySection.Flag("flag", "a simple flag");

void myCommandCallback();
// if "my_command" is passed as first argument to the executable myCommandCallback will be called from sargp::callCommands()
auto myCommand = sargp::Command{"my_command", "help text for that command", myCommandCallback};
auto myCommandSpecificParameter = myCommand.Flag("print_hello", "print hello");
auto myTextToPrint = myCommand.Parameter<std::vector<std::string>>({"some", "words"}, "words_to_print", "print some words");
void myCommandCallback() {
	std::cout << "executing \"my_command\"" << std::endl;
	if (myCommandSpecificParameter) {
		std::cout << "hello" << std::endl;
	}
	// if the compiler can infer a cast to the underlying type of the parameter it will do so
	// also: explicit casts are possible to get the value of a parameter or simply use .get()
	for (auto const& word : myTextToPrint.get()) {
		std::cout << word << " ";
	}
	std::cout << std::endl;
}

// choices (e.g., for enums) are also possible
enum class MyEnumType {Foo, Bar};
auto myChoice = sargp::Choice<MyEnumType>{MyEnumType::Foo, "my_enum",
	{{"Foo", MyEnumType::Foo}, {"Bar", MyEnumType::Bar}}, "a choice demonstration"
};
}
~~~

file main.cpp:
~~~
#include <sargparse/ArgumentParsing.h>
#include <sargparse/Parameter.h>
#include <iostream>

namespace {
auto printHelp  = sargp::Parameter<std::optional<std::string>>{{}, "help", "print this help add a string which will be used in a grep-like search through the parameters"};
}

int main(int argc, char** argv)
{
	// create you own bash completion with this helper
	if (std::string(argv[argc-1]) == "--bash_completion") {
		auto hint = sargp::compgen(argc-2, argv+1);
		std::cout << hint << " ";
		return 0;
	}

	// parse the arguments (excluding the application name) and fill all parameters/flags/choices with their respective values
	sargp::parseArguments(argc-1, argv+1);
	if (printHelp) { // print the help
		std::cout << sargp::generateHelpString(std::regex{".*" + printHelp.get().value_or("") + ".*"});
		return 0;
	}
	sargp::callCommands();
	return 0;
}
