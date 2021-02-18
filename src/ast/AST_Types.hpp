#pragma once
#include <string>

#define SET_VALUE(value) value

namespace llang::ast {
    struct AssignNode;
    struct BinaryOperationNode;
    struct ConstantNode;
    struct FunctionCallNode;
    struct FunctionDefNode;
    struct ProgramNode;
    struct StatementNode;
    struct UnaryOperationNode;
    struct VariableDeclNode;
    struct VariableRefNode;

    enum class AST_TYPE {
        SET_VALUE(AssignNode),
        SET_VALUE(BinaryOperationNode),
        SET_VALUE(ConstantNode),
        SET_VALUE(FunctionCallNode),
        SET_VALUE(FunctionDefNode),
        SET_VALUE(StatementNode),
        SET_VALUE(ProgramNode),
        SET_VALUE(UnaryOperationNode),
        SET_VALUE(VariableDeclNode),
        SET_VALUE(VariableRefNode),
        COUNT
    };

    std::string GetAstTypeName(AST_TYPE type);
}

#undef SET_VALUE
