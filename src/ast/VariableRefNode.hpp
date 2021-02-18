#pragma once
#include "StatementNode.hpp"
#include "VariableDeclNode.hpp"

namespace llang::ast
{
    /**
     * Represents a variable reference
    */
    struct VariableRefNode : public StatementNode {
        std::shared_ptr<VariableDeclNode> Var;

        VariableRefNode() :
            StatementNode(STATEMENT_TYPE::VAR_REF) {}

        AST_TYPE GetType() const override {
            return GET_AST_TYPE(VariableRefNode);
        }

        void ToString(std::string &str, const int tabLevel) const override {
        
        }
    };
}