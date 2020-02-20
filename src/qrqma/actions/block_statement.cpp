#include "block_statement.h"

#include "../overloaded.h"
#include "../finally.h"

namespace qrqma {
namespace actions {

void action<grammar::block_control_statement>::success(std::string& block_name, ContextP& inner_ctx, bool scoped, ContextP& outer_ctx) {
    auto ctx = inner_ctx.get();
    if (scoped) {
        outer_ctx->setBlock(block_name, Context::ContextBlock{ctx, [inner_ctx=std::move(inner_ctx)](){
            return (*inner_ctx)();
        }});
    } else {
        outer_ctx->setBlock(block_name, [inner_ctx=std::move(inner_ctx)](){
            return (*inner_ctx)();
        });
    }
    outer_ctx->addRenderToken([outer_ctx=outer_ctx.get(), block_name=std::move(block_name)]{
        return std::visit(detail::overloaded{
                [](Context::StaticText const &t)    -> Context::RenderOutput { return {t, false}; },
                [](Context::Renderable const &c)    -> Context::RenderOutput { return c(); },
                [outer_ctx](Context::ContextBlock const &cb) -> Context::RenderOutput {
                    auto oldParent = cb.context->getParentContext();
                    cb.context->setParentContext(outer_ctx);
                    internal::Finally f{[&]{ cb.context->setParentContext(oldParent); }};
                    return cb.renderable();
                },
                [](auto const&) -> Context::RenderOutput { return {"", true}; }},
            outer_ctx->getBlock(block_name)
        );
    });
}

}
}