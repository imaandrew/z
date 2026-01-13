#include "zctxt.h"
#include "ast.h"
#include "diagnostics.h"
#include "type.h"
#include "type_ref.h"

namespace z {
bool ZContext::resolve_unk_type(type::TypeRef& type) const {
    if (auto* unk_type = ty->get_as<type::UnknownType>(type)) {
        const auto ident = unk_type->get_ident()->get_id();
        if (const auto new_type = syms->get_type(ident); new_type) {
            type = *new_type;
            return true;
        }

        diag.emit(unk_type->get_ident()->tok.get_span(),
                  DiagnosticKind::UndeclaredType,
                  unk_type->get_ident()->to_string(strings.get()));
        return false;
    }

    return true;
}
} // namespace z
