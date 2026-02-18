#include "zctxt.h"
#include "diag/diagnostics.h"
#include "type/type.h"
#include "type/type_ref.h"

namespace z {
bool ZContext::resolve_unk_type(type::TypeRef& type) {
    if (auto* unk_type = ty->get_as<type::UnknownType>(type)) {
        const auto ident = unk_type->get_id();
        if (const auto new_type = syms->get_type(ident); new_type) {
            type = *new_type;
            return true;
        }

        diag.error(unk_type->get_span(), DiagnosticKind::UndeclaredType,
                   strings->get_string(unk_type->get_id()));
        return false;
    }

    return true;
}
} // namespace z
