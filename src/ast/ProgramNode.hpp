#pragma once
#include <string>
#include "Node.hpp"
#include "../CppReflection.hpp"
#include "AST_Types.hpp"

namespace llang {
    namespace symbol_table {
        struct SymbolTableScope;
    }

    namespace ast {
        /**
         * Represents a whole program.
         * It is the root node of the tree.
         **/
        struct ProgramNode : public Node {
            std::shared_ptr<symbol_table::SymbolTableScope> GlobalScope;
            const std::string ModuleName;

            ProgramNode(const std::string& fileName, const std::string& moduleName) : ModuleName(moduleName) {
                FileName = fileName;
            }

            AST_TYPE GetType() const override {
                return GET_AST_TYPE(ProgramNode);
            }

            void ToString(std::string& str, const int tabLevel) const override {
                auto tabs = GetTabs(tabLevel);
                str += tabs + GetNodeBeginChar(tabLevel) + GET_TYPE_NAME(ProgramNode) + ": " + FileName;

                const int childTabLevel = tabLevel + 1;
                for (auto child : children) {
                    str += "\n";
                    child->ToString(str, childTabLevel);
                }
            }
        };
    }
}