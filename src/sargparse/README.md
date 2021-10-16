# sargparse
A very easy to use argument parser

sargparse is intended to be used in very modular software where parameters can reside all over the source files.

## Features:
- single value Parameters, Parameters with multiple values, Flags (Parameters with zero values)
- Sections (parameters can be devided in sections (groups)
- Commands and Subcommands, Parameters and Flags can be bound to certain (Sub-)commands
- Support for values with suffix (e.g.: `k` for x1000 and `ki` for 1024, or `0.5rad` which will be converted to `0.5*PI`)
- autocompletion for bash and zsh (includes generic completion scripts)
- help page generation
- man page generation


## Example Calls
Some example calls:
```
./exampleSargparse --help
./exampleSargparse --man
./exampleSargparse my_command
./exampleSargparse add file1.txt file2.text
./exampleSargparse --my_enum Bar
./exampleSargparse --my_enum Foo
./exampleSargparse --my_enum Bar
./exampleSargparse --mySection.flag
./exampleSargparse --mySection.double 3.5
./exampleSargparse --mySection.double 0.5rad
./exampleSargparse --mySection.flag --mySection.integer 5
./exampleSargparse --mySection.string "hallo welt"
./exampleSargparse --mySection.files file1.txt file2.txt
```

## Example Code
Check [`example`](https://github.com/gottliebtfreitag/sargparse/tree/example) branch for a more detailed example.

Here is a small example about its usage:

File foo.cpp:
~~~
#include <sargparse/sargparse.h>
#include <iostream>

namespace {

// all parameters comming from this section have "mySection." as their name prefix
auto mySection = sargp::Section{"mySection"};
auto myIntParam    = mySection.Parameter<int>(123, "integer", "an integer argument");

void myCommandCallback() {
	std::cout << "executing \"my_command\"\n;
	std::cout << "mySection.integer has value" << *myIntParam << "\n";
}
auto myCommand = sargp::Command{"my_command", "help text for that command", myCommandCallback};

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
#include <sargparse/sargparse.h>
#include <iostream>

namespace {
auto printHelp  = sargp::Parameter<std::optional<std::string>>{{}, "help", "print this help add a string which will be used in a grep-like search through the parameters"};
}

int main(int argc, char** argv)
{
	// create you own bash completion with this helper
	if (std::string(argv[argc-1]) == "--bash_completion") {
		auto hint = sargp::compgen(argc-2, argv+1);
		std::cout << hint << "\n";
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
