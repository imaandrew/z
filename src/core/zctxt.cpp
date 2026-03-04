#include "zctxt.h"
#include "diag/diagnostics.h"
#include "type/type.h"
#include "type/type_ref.h"
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace z {
bool ZContext::resolve_unk_type(type::TypeRef& root_type) {
    std::unordered_map<type::TypeRef, type::TypeRef> resolved_types;

    std::function<bool(type::TypeRef&)> resolve;
    resolve = [&](type::TypeRef& ref) -> bool {
        // Prevent infinite recursion if we have self-referential types
        if (auto it = resolved_types.find(ref); it != resolved_types.end()) {
            ref = it->second;
            return true;
        }

        auto orig_type = ref;
        resolved_types[orig_type] = ref;

        if (auto* unk_type = ty->get_as<type::UnknownType>(ref)) {
            const auto ident = unk_type->get_id();
            const auto new_type = syms->get_type(ident);
            if (!new_type) {
                diag.error(unk_type->get_span(), DiagnosticKind::UndeclaredType,
                           strings->get_string(unk_type->get_id()));
                return false;
            }

            ref = *new_type;
        } else if (auto* ptr_type = ty->get_as<type::PointerType>(ref)) {
            auto inner = ptr_type->get_type();
            if (!resolve(inner))
                return false;

            if (inner != ptr_type->get_type()) {
                ref = ty->make<type::PointerType>(inner);
            }
        } else if (auto* array_type = ty->get_as<type::ArrayType>(ref)) {
            auto inner = array_type->get_type();
            if (!resolve(inner))
                return false;

            if (inner != array_type->get_type()) {
                ref = ty->make<type::ArrayType>(inner, array_type->get_size());
            }
        } else if (auto* func_type = ty->get_as<type::FunctionType>(ref)) {
            bool changed = false;
            std::vector<type::TypeRef> params;
            for (auto param : func_type->get_params()) {
                if (!resolve(param))
                    return false;

                if (param != func_type->get_params().at(params.size()))
                    changed = true;

                params.push_back(param);
            }

            auto ret = func_type->get_return_val();
            if (!resolve(ret))
                return false;

            if (ret != func_type->get_return_val())
                changed = true;

            if (changed) {
                ref = ty->make<type::FunctionType>(std::move(params), ret);
            }
        } else if (auto* tuple_type = ty->get_as<type::TupleType>(ref)) {
            auto old_types = tuple_type->get_types();
            auto [first, second] = old_types;
            if (!resolve(first))
                return false;

            if (!resolve(second))
                return false;

            if (first != old_types.first || second != old_types.second) {
                ref = ty->make<type::TupleType>(first, second);
            }
        } else if (auto* struct_type = ty->get_as<type::StructType>(ref)) {
            // Since struct and enum types aren't internable, we are able to
            // change
            // which types they hold internally without having to worry about
            // messing up their hash because equality is only based on their
            // name
            auto& fields = struct_type->get_fields_mut();
            for (auto& [id, field] : fields) {
                if (!resolve(field.first))
                    return false;
            }

            auto& funcs = struct_type->get_funcs_mut();
            for (auto& [id, func] : funcs) {
                if (!resolve(func.first))
                    return false;
            }
        } else if (auto* enum_type = ty->get_as<type::EnumType>(ref)) {
            auto& fields = enum_type->get_fields_mut();
            for (auto& [id, types] : fields) {
                for (auto& t : types) {
                    if (!resolve(t))
                        return false;
                }
            }
        }

        resolved_types[orig_type] = ref;
        return true;
    };

    return resolve(root_type);
}
} // namespace z
