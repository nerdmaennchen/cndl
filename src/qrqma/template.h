#pragma once

#include <map>
#include <memory>
#include <string_view>

#include "loader.h"
#include "symbol.h"

namespace qrqma {

struct Template {
    Template(std::string_view input, symbol::SymbolTable symbols={}, TemplateLoader loader=defaultLoader(), symbol::BlockTable blocks={});
    Template(Template&&);
    Template& operator=(Template&&);
    
    ~Template();

    std::string operator()(symbol::SymbolTable symbols={}) const;

  private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace qrqma
