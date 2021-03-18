#include "AstBuilder.hpp"
#include "Node.hpp"
#include "ProgramNode.hpp"
#include "AssignNode.hpp"
#include "VariableDefNode.hpp"
#include "VariableRefNode.hpp"
#include "FunctionDefNode.hpp"
#include "UnaryOperationNode.hpp"
#include "BinaryOperationNode.hpp"
#include "ConstantNode.hpp"
#include <sstream>
#include <memory>

using namespace llang;
using namespace ast;

antlrcpp::Any AstBuilder::visitSourceFile(LlamaLangParser::SourceFileContext *context) {
    context->AstNode = ASTree;

    // add program source to the program tree
    for( auto child : context->children ) {
        auto result = visit(child);
        if( result.isNotNull() ) {
            auto resultVal = result.as<Node::ChildType>();
            ASTree->children.push_back(result);
        }
    }

    return ASTree;
}

antlrcpp::Any AstBuilder::visitFunctionDef(LlamaLangParser::FunctionDefContext *context) {
    auto funcNode = std::make_shared<FunctionDefNode>(); 
    context->AstNode = funcNode;

    funcNode->FileName = FileName;
    funcNode->Line = context->start->getLine();
    funcNode->Name = context->IDENTIFIER()->getText();
    funcNode->ReturnType = context->type_()->getText();

    // Add function scope to parent scope and make it current
    currentScope = currentScope->addChild(symbol_table::SCOPE_TYPE::FUNC, funcNode->Name, funcNode).back();


    /* Get the result of the last child visited (block) */
    auto blockAny = visitChildren(context);

    if( blockAny.is<FunctionDefNode::BlockType *>() ) {
        funcNode->Block = *blockAny.as<FunctionDefNode::BlockType *>();
    }

    currentScope = currentScope->Parent;

    return CastNode<Node>(funcNode);
}

antlrcpp::Any AstBuilder::visitSignature(LlamaLangParser::SignatureContext *context) {
    auto parentContext = (LlamaLangParseContext *) context->parent;
    // parent context ASTnode is a FunctionDefNode
    context->AstNode = parentContext->AstNode;

    // set parameters
    visitChildren(context);

    return nullptr;
}

antlrcpp::Any AstBuilder::visitParameters(LlamaLangParser::ParametersContext *context) {
    auto parentContext = (LlamaLangParseContext *) context->parent;
    auto funcNode = CastNode<FunctionDefNode>(parentContext->AstNode);

    auto parameters = context->parameterDecl();

    for( auto paramContext : parameters ) {
        if( paramContext->isEmpty() || paramContext->exception != nullptr )
            continue;

        auto param = std::make_shared<VariableDefNode>();
        param->FileName = FileName;
        param->Line = context->start->getLine();
        param->Name = paramContext->IDENTIFIER()->getText();
        param->VarType = paramContext->type_()->getText();
        funcNode->Parameters.push_back(param);
    }

    // It is not necesary to visit children after this visit
    return nullptr;
}

antlrcpp::Any AstBuilder::visitBlock(LlamaLangParser::BlockContext *context) {
    // returns the std::vector from visitStatementList
    return visitChildren(context);
}

antlrcpp::Any AstBuilder::visitStatementList(LlamaLangParser::StatementListContext *context) {
    auto statementContexts = context->statement();

    auto stmntList = new FunctionDefNode::BlockType();

    for( auto statementContext : statementContexts ) {
        if( statementContext->isEmpty() || statementContext->exception != nullptr )
            continue;

        auto stmntAny = visitChildren(statementContext);
        if( stmntAny.is<FunctionDefNode::StatementType>() ) {
            auto stmnt = stmntAny.as<FunctionDefNode::StatementType>();
            stmntList->push_back(stmnt);
        }
    }

    return stmntList;
}

antlrcpp::Any AstBuilder::visitReturnStmt(LlamaLangParser::ReturnStmtContext *context) {
    auto retStmnt = std::make_shared<UnaryOperationNode>(UNARY_STATEMENT_TYPE::RETURN);
    retStmnt->FileName = FileName;
    retStmnt->Line = context->start->getLine();

    context->AstNode = retStmnt;

    auto rightAny = visitChildren(context);

    if( rightAny.isNotNull() ) {
        auto rightStmnt = rightAny.as<std::shared_ptr<StatementNode>>();
        retStmnt->Right = rightStmnt;
    }

    return CastNode<StatementNode>(retStmnt);
}

antlrcpp::Any AstBuilder::visitExpression(LlamaLangParser::ExpressionContext *context) {
    if( context->right && context->left ) {
        auto rightNodeAny = visit(context->right);
        auto leftNodeAny = visit(context->left);
        if( rightNodeAny.isNull() || leftNodeAny.isNull() )
            return nullptr;

        if( rightNodeAny.is<std::shared_ptr<ConstantNode>>() && leftNodeAny.is<std::shared_ptr<ConstantNode>>() ) {
            auto rightNode = rightNodeAny.as<std::shared_ptr<ConstantNode>>();
            auto leftNode = leftNodeAny.as<std::shared_ptr<ConstantNode>>();
            
            if( rightNode->ConstType == CONSTANT_TYPE::STRING || leftNode->ConstType == CONSTANT_TYPE::STRING )
                goto runtime_expr;
            
            // Initialize it in I64
            auto resultNode = std::make_shared<ConstantNode>(CONSTANT_TYPE::I64);
            // TODO: check for overflow and handle it
            bool checkOverflow;

            if( context->PLUS() ) {
                // ADDITION EXPRESSION
                resultNode->ConstType = GetResultType(BINARY_OPERATION::ADD, rightNode, leftNode, checkOverflow);
                if( resultNode->ConstType <= CONSTANT_TYPE::I64 ) {
                    int64_t rightVal = std::stoll(rightNode->Value);
                    int64_t leftVal = std::stoll(leftNode->Value);
                    auto resultVal = rightVal + leftVal;
                    resultNode->Value = std::to_string(resultVal);
                } else {
                    double rightVal = std::stod(rightNode->Value);
                    double leftVal = std::stod(leftNode->Value);
                    auto resultVal = rightVal + leftVal;
                    resultNode->Value = std::to_string(resultVal);
                }
            } else if( context->MINUS() ) {
                // SUBSTRACTION EXPRESSION
                resultNode->ConstType = GetResultType(BINARY_OPERATION::SUB, rightNode, leftNode, checkOverflow);
                if( resultNode->ConstType <= CONSTANT_TYPE::I64 ) {
                    int64_t rightVal = std::stoll(rightNode->Value);
                    int64_t leftVal = std::stoll(leftNode->Value);
                    auto resultVal = leftVal - rightVal;
                    resultNode->Value = std::to_string(resultVal);
                } else {
                    double rightVal = std::stod(rightNode->Value);
                    double leftVal = std::stod(leftNode->Value);
                    auto resultVal = leftVal - rightVal;
                    resultNode->Value = std::to_string(resultVal);
                }

            } else if( context->STAR() ) {
                // MULTIPLICATION EXPRESSION
                resultNode->ConstType = GetResultType(BINARY_OPERATION::MUL, rightNode, leftNode, checkOverflow);
                if( resultNode->ConstType <= CONSTANT_TYPE::I64 ) {
                    int64_t rightVal = std::stoll(rightNode->Value);
                    int64_t leftVal = std::stoll(leftNode->Value);
                    auto resultVal = rightVal * leftVal;
                    resultNode->Value = std::to_string(resultVal);
                } else {
                    double rightVal = std::stod(rightNode->Value);
                    double leftVal = std::stod(leftNode->Value);
                    auto resultVal = rightVal * leftVal;
                    resultNode->Value = std::to_string(resultVal);
                }
            } else if( context->DIV() ) {
                // DIVITION EXPRESSION
                resultNode->ConstType = GetResultType(BINARY_OPERATION::DIV, rightNode, leftNode, checkOverflow);
                if( resultNode->ConstType <= CONSTANT_TYPE::I64 ) {
                    int64_t rightVal = std::stoll(rightNode->Value);
                    int64_t leftVal = std::stoll(leftNode->Value);
                    auto resultVal = leftVal / rightVal;
                    resultNode->Value = std::to_string(resultVal);
                } else {
                    double rightVal = std::stod(rightNode->Value);
                    double leftVal = std::stod(leftNode->Value);
                    auto resultVal = leftVal / rightVal;
                    resultNode->Value = std::to_string(resultVal);
                }
            } else if( context->MOD() ) {
                // MODULOUS EXPRESSION
                resultNode->ConstType = GetResultType(BINARY_OPERATION::MOD, rightNode, leftNode, checkOverflow);
                if( resultNode->ConstType <= CONSTANT_TYPE::I64 ) {
                    int64_t rightVal = std::stoll(rightNode->Value);
                    int64_t leftVal = std::stoll(leftNode->Value);
                    auto resultVal = leftVal % rightVal;
                    resultNode->Value = std::to_string(resultVal);
                } else {
                    // MOD not suported with floating point operands
                    // send the nodes as binaryOp and let the SemanticAnalizer report the error
                    goto runtime_expr;
                }
            }
            return CastNode<StatementNode>(resultNode);
        } else {
runtime_expr:
            auto rightNode = rightNodeAny.as<std::shared_ptr<StatementNode>>();
            auto leftNode = leftNodeAny.as<std::shared_ptr<StatementNode>>();
            std::shared_ptr<BinaryOperationNode> binOpNode;

            if( context->PLUS() ) {
                // ADDITION EXPRESSION
                binOpNode = std::make_shared<BinaryOperationNode>(BINARY_OPERATION::ADD);
            } else if( context->MINUS() ) {
                // SUBSTRACTION EXPRESSION
                binOpNode = std::make_shared<BinaryOperationNode>(BINARY_OPERATION::SUB);
            } else if( context->STAR() ) {
                // MULTIPLICATION EXPRESSION
                binOpNode = std::make_shared<BinaryOperationNode>(BINARY_OPERATION::MUL);
            } else if( context->DIV() ) {
                // DIVITION EXPRESSION
                binOpNode = std::make_shared<BinaryOperationNode>(BINARY_OPERATION::DIV);
            } else if( context->MOD() ) {
                // MODULOUS EXPRESSION
                binOpNode = std::make_shared<BinaryOperationNode>(BINARY_OPERATION::MOD);
            }

            if( binOpNode != nullptr ) {
                binOpNode->Right = rightNode;
                binOpNode->Left = leftNode;
            }

            return CastNode<StatementNode>(binOpNode);
        }
    } else {
        // Unary || Primary expression
        // returns ConstantNode, UnaryOperationNode
        return visitChildren(context);
    }
}

antlrcpp::Any AstBuilder::visitVarDef(LlamaLangParser::VarDefContext *context) {
    // variable definition
    auto varDefNode = std::make_shared<VariableDefNode>();
    varDefNode->FileName = FileName;
    varDefNode->Line = context->start->getLine();
    varDefNode->Name = context->IDENTIFIER()->getText();
    varDefNode->VarType = context->type_()->getText();
    varDefNode->isGlobal = currentScope == globalScope;

    if( context->ASSIGN() ) {
        auto assignmentStmnt = std::make_shared<AssignNode>();
        assignmentStmnt->Left = std::make_shared<VariableRefNode>();
        assignmentStmnt->Left->WasFound = true;
        assignmentStmnt->Left->Var = varDefNode;
        assignmentStmnt->Right = visit(context->expressionList());
        varDefNode->assignmentStmnt = assignmentStmnt;
    }

    // Add symbol
    currentScope->addSymbol(varDefNode->Name, varDefNode);

    return CastNode<StatementNode>(varDefNode);
}

antlrcpp::Any AstBuilder::visitAssignment(LlamaLangParser::AssignmentContext *context) {
    auto assignmentStmnt = std::make_shared<AssignNode>();
    
    auto varName = context->IDENTIFIER()->getText();
    auto varRefNode = std::make_shared<VariableRefNode>();
    varRefNode->SetAsNotFound(varName);

    assignmentStmnt->Left = varRefNode;
    assignmentStmnt->Right = visit(context->expressionList());

    return CastNode<StatementNode>(assignmentStmnt);
}

antlrcpp::Any AstBuilder::visitOperandName(LlamaLangParser::OperandNameContext *context) {
    auto varName = context->IDENTIFIER()->getText();

    auto varRefNode = std::make_shared<VariableRefNode>();
    varRefNode->SetAsNotFound(varName);
    
    return varRefNode;
}

/*
* Can be:
* -(IDENTIFIER|NUMBER|STRING)
* +(IDENTIFIER|NUMBER|STRING)
* (IDENTIFIER|NUMBER|STRING)++
* (IDENTIFIER|NUMBER|STRING)--
*/
antlrcpp::Any AstBuilder::visitUnaryExpr(LlamaLangParser::UnaryExprContext *context) {
    auto childAny = visitChildren(context);

    if( childAny.is<std::shared_ptr<ConstantNode>>() ) {
        auto constChild = childAny.as<std::shared_ptr<ConstantNode>>();
        if( context->unaryOp()->MINUS() ) {
            constChild->Value = "-" + constChild->Value;
        }
        return constChild;
    }

    std::shared_ptr<UnaryOperationNode> unaryStmnt;

    /*
    if( context->unaryOp()->MINUSMINUS() )
        unaryStmnt = std::make_shared<UnaryOperationNode>(UNARY_STATEMENT_TYPE::DECREMENT);
    else if( context->unaryOp()->PLUSPLUS() )
        unaryStmnt = std::make_shared<UnaryOperationNode>(UNARY_STATEMENT_TYPE::INCREMENT);
    */

    unaryStmnt->Right = childAny.as<std::shared_ptr<StatementNode>>();
    return unaryStmnt;
}

antlrcpp::Any AstBuilder::visitBasicLit(LlamaLangParser::BasicLitContext *context) {
    std::shared_ptr<ConstantNode> constantNode = nullptr;

    if( context->integer() != nullptr ) {
        auto textStream = std::istringstream(context->integer()->getText());
        uint64_t number;
        textStream >> number;

        CONSTANT_TYPE intType;
        if( number <= std::numeric_limits<uint8_t>::max() && number >= std::numeric_limits<uint8_t>::min() ) {
            intType = CONSTANT_TYPE::I8;
        } else if( number <= std::numeric_limits<uint16_t>::max() && number >= std::numeric_limits<uint16_t>::min() ) {
            intType = CONSTANT_TYPE::I16;
        } else if( number <= std::numeric_limits<uint32_t>::max() && number >= std::numeric_limits<uint32_t>::min() ) {
            intType = CONSTANT_TYPE::I32;
        } else if( textStream.good() ) {
            intType = CONSTANT_TYPE::I64;
        } else {
            return nullptr;
        }
        constantNode = std::make_shared<ConstantNode>(intType);
        constantNode->Value = context->integer()->getText();
    } else if( context->floatingPoint() != nullptr ) {
        auto text = context->floatingPoint()->getText();
        CONSTANT_TYPE floatingPointType;
        if( tolower(text.back()) == 'f' ) {
            text.pop_back();
            floatingPointType = CONSTANT_TYPE::FLOAT;
        } else
            floatingPointType = CONSTANT_TYPE::DOUBLE;

        constantNode = std::make_shared<ConstantNode>(floatingPointType);
        constantNode->Value = text;
    } else if( context->RUNE_LIT() != nullptr ) {
        constantNode = std::make_shared<ConstantNode>(CONSTANT_TYPE::CHAR);
        constantNode->Value = context->RUNE_LIT()->getText();
    } else if( context->string_() ) {
        constantNode = std::make_shared<ConstantNode>(CONSTANT_TYPE::STRING);
        constantNode->Value = context->string_()->getText();
    }

    if( constantNode != nullptr ) {
        constantNode->FileName = FileName;
        constantNode->Line = context->start->getLine();
        return CastNode<StatementNode>(constantNode);
    }

    return nullptr;
}



/*****************************
* TREE BEHAVIOUR OVERRIDES
*****************************/

antlrcpp::Any AstBuilder::visitChildren(antlr4::tree::ParseTree *node) {
    antlrcpp::Any result = defaultResult();
    for( size_t i = 0; i < node->children.size(); i++ ) {
        auto child = node->children[i];
        if( !shouldVisitNextChild(child, result) ) {
            continue;
        }

        antlrcpp::Any childResult = child->accept(this);
        result = this->aggregateResult(result, childResult);
    }

    return result;
}

bool AstBuilder::shouldVisitNextChild(antlr4::tree::ParseTree *node, const antlrcpp::Any &currentResult) {
    auto terminalNode = dynamic_cast<antlr4::tree::TerminalNode *>( node );

    if( terminalNode )
        return false;

    auto context = dynamic_cast<antlr4::ParserRuleContext *>( node );
    if( context )
        return context->children.size() > 0 || context->isEmpty() || context->exception != nullptr;

    return false;
}

antlrcpp::Any &AstBuilder::aggregateResult(antlrcpp::Any &result, antlrcpp::Any &nextResult) {
    if( result.isNotNull() && nextResult.isNull() )
        return result;
    return nextResult;
}