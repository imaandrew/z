#include "type.h"
#include "ast.h"
#include "type_arena.h"
#include "type_ref.h"
#include "zctxt.h"
#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace z::type {

ArrayType::ArrayType(TypeRef type) : Type(Kind), type(type) {};
ArrayType::ArrayType(TypeRef type, std::shared_ptr<ast::Expr> size)
    : Type(Kind), type(type), size(std::move(size)) {};
ArrayType::~ArrayType() = default;

TypeArena::TypeArena() {
    types.reserve(16);
    types.push_back(std::make_unique<InvalidType>());
    types.push_back(std::make_unique<IntegerType>(8, true));
    types.push_back(std::make_unique<IntegerType>(16, true));
    types.push_back(std::make_unique<IntegerType>(32, true));
    types.push_back(std::make_unique<IntegerType>(64, true));
    types.push_back(std::make_unique<IntegerType>(8, false));
    types.push_back(std::make_unique<IntegerType>(16, false));
    types.push_back(std::make_unique<IntegerType>(32, false));
    types.push_back(std::make_unique<IntegerType>(64, false));
    types.push_back(std::make_unique<FloatType>(32));
    types.push_back(std::make_unique<FloatType>(64));
    types.push_back(std::make_unique<BooleanType>());
    types.push_back(std::make_unique<StringType>());
    types.push_back(std::make_unique<CharType>());
    types.push_back(std::make_unique<VoidType>());
    types.push_back(std::make_unique<InvalidType>());
}

void PointerType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "PointerType { type: ";
    ctxt->ty->get(type)->dump(ctxt, stream);
    stream << " }";
}

std::string PointerType::basic_name(ZContext* ctxt) const {
    return ctxt->ty->get(type)->basic_name(ctxt) + "*";
}

void ArrayType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "ArrayType { type: ";
    ctxt->ty->get(type)->dump(ctxt, stream);
    stream << ", size: ";
    // TODO
    stream << " }";
}

std::string ArrayType::basic_name(ZContext* ctxt) const {
    return std::format("[{}]", ctxt->ty->get(type)->basic_name(ctxt));
}

void FunctionType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "FunctionType { params: [";
    if (!params.empty()) {
        for (size_t i = 0; i < params.size() - 1; i++) {
            ctxt->ty->get(params[i])->dump(ctxt, stream);
            stream << ", ";
        }
        ctxt->ty->get(params.back())->dump(ctxt, stream);
    }

    stream << "], return_val: ";
    if (return_val.is_valid()) {
        ctxt->ty->get(return_val)->dump(ctxt, stream);
    }
    stream << " }";
}

std::string FunctionType::basic_name(ZContext* ctxt) const {
    std::string s = "(";
    if (!params.empty()) {
        for (size_t i = 0; i < params.size() - 1; i++) {
            s += ctxt->ty->get(params[i])->basic_name(ctxt) + ", ";
        }
        s += ctxt->ty->get(params.back())->basic_name(ctxt);
    }
    s += ")";

    if (return_val.is_valid()) {
        s += "(" + ctxt->ty->get(return_val)->basic_name(ctxt) + ")";
    } else {
        s += "()";
    }

    return s;
}

void StructType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "StructType { name: " << ctxt->strings->get_string(name) << " }";
}

std::string StructType::basic_name(ZContext* ctxt) const {
    return ctxt->strings->get_string(name);
}

void EnumType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "EnumType { name: " << ctxt->strings->get_string(name) << " }";
}

std::string EnumType::basic_name(ZContext* ctxt) const {
    return ctxt->strings->get_string(name);
}

void EnumVariantType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "EnumVariantType { parent: "
           << ctxt->strings->get_string(parent_enum) << " }";
}

std::string EnumVariantType::basic_name(ZContext* ctxt) const {
    return ctxt->strings->get_string(parent_enum);
}

void TupleType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << basic_name(ctxt);
}

std::string TupleType::basic_name(ZContext* ctxt) const {
    return std::format("({}, {})", ctxt->ty->get(first)->basic_name(ctxt),
                       ctxt->ty->get(second)->basic_name(ctxt));
}

void TypeType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "TypeType { ";
    ctxt->ty->get(internal_type)->dump(ctxt, stream);
    stream << " }";
}

std::string TypeType::basic_name(ZContext* ctxt) const {
    return std::format("type({})",
                       ctxt->ty->get(internal_type)->basic_name(ctxt));
}

void TraitType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "TraitType { name: " << ctxt->strings->get_string(name) << " }";
}

std::string TraitType::basic_name(ZContext* ctxt) const {
    return std::format("trait({})", ctxt->strings->get_string(name));
}

void TempType::dump(ZContext* ctxt, std::ostream& stream) const {
    stream << "TempType { name: " << ctxt->strings->get_string(name) << " }";
}

std::string TempType::basic_name(ZContext* ctxt) const {
    return std::format("temp({})", ctxt->strings->get_string(name));
}

} // namespace z::type
