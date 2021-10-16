#include "function_call.h"
#include "types.h"
#include "convert.h"
#include "../demangle.h"

#include <stdexcept>

namespace qrqma {
namespace actions {

using  CE = types::ConstantExpression;
using NCE = types::NonconstantExpression;

void action<grammar::ops::call>::success(std::string const& in, ContextP &context, ContextP &argsC) {
    auto ftor_e = context->popExpression();
    auto args_r = argsC->popAllExpressions();
    
    bool allconst = std::holds_alternative<CE>(ftor_e) and
        std::all_of(begin(args_r), end(args_r), [](auto const& e) { return std::holds_alternative<CE>(e); });

    if (allconst) {
		std::optional<symbol::Function> ftor;
		try {
			ftor.emplace(std::any_cast<symbol::Function>(std::get<CE>(ftor_e).eval()));
		} catch (std::bad_any_cast const& e) {
			throw std::invalid_argument("cannot perform the call: \"" + in + "\": expression is not callable");
		}
        auto const& inArgTypes = ftor->argTypes;
        if (args_r.size() != inArgTypes.size()) {
            throw std::invalid_argument("cannot perform the call: \"" + in + "\": invalid numer of arguments\nexpected " + std::to_string(inArgTypes.size()) + " got " + std::to_string(args_r.size()));
        }
        std::vector<std::any> args;
        for (std::size_t i{0}; i < args_r.size(); ++i) {
            auto val = std::get<CE>(args_r[i]).eval();
            try {
                args.emplace_back(convert(val, *inArgTypes[i]));
            } catch (std::exception const& e) {
                std::throw_with_nested(
                    std::runtime_error("in function call: \"" + in + "\": cannot convert arg no " + std::to_string(i) 
                    + " from type \"" + internal::demangle(val.type()) 
                    + "\" to type: \"" + internal::demangle(*inArgTypes[i]) + "\""
                ));
            }
        }
        context->pushExpression(NCE{
            [ftor=std::move(*ftor), args=std::move(args)] {
                return ftor(args);
            }
        });
    } else {
        context->pushExpression(NCE{
            [ftor_e=std::move(ftor_e), args_r=std::move(args_r), in=std::move(in), argsC=std::move(argsC)] {
				std::optional<symbol::Function> ftor;
				try {
					ftor.emplace(std::visit([](auto const& e) { return std::any_cast<symbol::Function>(e.eval()); }, ftor_e));
				} catch (std::bad_any_cast const& e) {
					throw std::invalid_argument("cannot perform the call: \"" + in + "\": expression is not callable");
				}
                auto const& inArgTypes = ftor->argTypes;
                if (args_r.size() != inArgTypes.size()) {
                    throw std::invalid_argument("cannot perform the call: \"" + in + "\": invalid numer of arguments\nexpected " + std::to_string(inArgTypes.size()) + " got " + std::to_string(args_r.size()));
                }
                std::vector<std::any> args;
                for (std::size_t i{0}; i < args_r.size(); ++i) {
                    auto val = std::visit([](auto const& e) {
                        return e.eval();
                    }, args_r[i]);
                    try {
                        args.emplace_back(convert(val, *inArgTypes[i]));
                    } catch (std::exception const& e) {
                        std::throw_with_nested(
                            std::runtime_error("in function call: \"" + in + "\": cannot convert arg no " + std::to_string(i) 
                            + " from type \"" + internal::demangle(val.type()) 
                            + "\" to type: \"" + internal::demangle(*inArgTypes[i]) + "\""
                        ));
                    }
                }
                return (*ftor)(args);
            }
        });
    }
}


} // namespace actions
} // namespace qrqma
