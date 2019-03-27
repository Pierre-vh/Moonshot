﻿//----------------------------------------------------------------------------//
// This file is a part of The Moonshot Project.
// See LICENSE.txt for license info.
// File : ASTDumper.cpp
// Author : Pierre van Houtryve
//----------------------------------------------------------------------------//

#include "Fox/AST/ASTDumper.hpp"
#include "Fox/AST/Identifier.hpp"
#include "Fox/AST/Type.hpp"
#include "Fox/AST/Types.hpp"
#include "Fox/Common/Errors.hpp"
#include "Fox/Common/SourceManager.hpp"
#include "utfcpp/utf8.hpp"
#include <sstream>
#include <iostream>
#include <string>

#define INDENT "    "
#define OFFSET_INDENT "\t"

using namespace fox;

// Helper functions
namespace {
	template<typename TyA,typename TyB>
	std::string makeKeyPairDump(TyA label, TyB value) {
		std::ostringstream ss;
		ss << "<" << label << ":";
    if(std::is_pointer<TyB>::value)
      ss << "0x" << value << ">";
    else 
      ss << value << ">";
		return ss.str();
	}
}

ASTDumper::ASTDumper(SourceManager& srcMgr,
                     std::ostream& out,
                     std::uint8_t offsettabs)
    : out(out), offsetTabs_(offsettabs), srcMgr_(&srcMgr) {
  recalculateOffset();
}

ASTDumper::ASTDumper(std::ostream& out, std::uint8_t offsettabs):
  srcMgr_(nullptr), out(out), offsetTabs_(offsettabs) {}

void ASTDumper::visitBinaryExpr(BinaryExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " " << getOperatorDump(node)
             << "\n";

  // Print LHS
  indent();
  visit(node->getLHS());
  dedent();

  // Print RHS
  indent();
  visit(node->getRHS());
  dedent();
}

void ASTDumper::visitCastExpr(CastExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " "
    << makeKeyPairDump("to", toString(node->getCastTypeLoc())) << "\n";
  indent();
  visit(node->getExpr());
  dedent();
}

void ASTDumper::visitUnaryExpr(UnaryExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " " << getOperatorDump(node) << "\n";
  indent();
  visit(node->getExpr());
  dedent();
}

void ASTDumper::visitArraySubscriptExpr(ArraySubscriptExpr* node) {
  dumpLine() << getBasicExprInfo(node) << '\n';

  indent();
  visit(node->getBase());
  dedent();

  indent();
  visit(node->getIndex());
  dedent();
}

void ASTDumper::visitMemberOfExpr(MemberOfExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " ." << node->getMemberID() << "\n";
  indent();
  visit(node->getExpr());
  dedent();
}

void ASTDumper::visitDeclRefExpr(DeclRefExpr* node) {
  NamedDecl* ref = node->getDecl();
  // FIXME: should work with invalid/ill formed ASTs
  assert(ref && "no referenced decl");
  dumpLine() << getBasicExprInfo(node) << " "
    << ref->getIdentifier() << " "
    << makeKeyPairDump("decl", (void*)node->getDecl())
    << "\n";
}

void ASTDumper::visitUnresolvedDeclRefExpr(UnresolvedDeclRefExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " "
    << node->getIdentifier() << "\n";
}

void ASTDumper::visitCallExpr(CallExpr* node) {
  dumpLine() << getBasicExprInfo(node) << '\n';

  // Print Base
  indent();
  visit(node->getCallee());
  dedent();

  // Print Args
  for (Expr* arg : node->getArgs()) {
    indent();
    visit(arg);
    dedent();
  }
}

namespace {
  void dumpNormalized(char ch, std::ostream& out, bool forChar = true) {
    switch (ch) {
      case 0:
        out << "\\0";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\'':
        if(forChar)
          out << "\\'";
        else 
          out << "'"; // Don't escape single quotes for strings
        break;
      case '\"':
        if(forChar)
          out << '"'; // Don't escape double quotes for chars
        else 
          out << "\\\"";
        break;
      default:
        out << ch;
        break;
    }
  }

  void dumpNormalized(string_view str, std::ostream& out) {
    for (char ch : str)
      dumpNormalized(ch, out, false);
  }
}

void ASTDumper::visitCharLiteralExpr(CharLiteralExpr* node) {
  std::string res;
  dumpLine() << getBasicExprInfo(node) << " '";
  dumpNormalized(node->getValue(), out);
  out << "' (" << +(node->getValue()) << ")\n";
}

void ASTDumper::visitStringLiteralExpr(StringLiteralExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " \"";
  dumpNormalized(node->getValue(), out);
  out << "\"\n";
}

void ASTDumper::visitIntegerLiteralExpr(IntegerLiteralExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " " << node->getValue() << "\n";
}

void ASTDumper::visitDoubleLiteralExpr(DoubleLiteralExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " " << node->getValue() << "\n";
}

void ASTDumper::visitBoolLiteralExpr(BoolLiteralExpr* node) {
  dumpLine() << getBasicExprInfo(node) << " "
    << (node->getValue() ? "true" : "false") << "\n";
}

void ASTDumper::visitArrayLiteralExpr(ArrayLiteralExpr* node) {
  std::size_t elemcount = node->numElems();

  dumpLine() << getBasicExprInfo(node) << " "
             << makeKeyPairDump("size", elemcount) << "\n";

  indent();
  for (Expr* expr : node->getExprs())
    visit(expr);
  dedent();
}

void ASTDumper::visitErrorExpr(ErrorExpr* expr) {
  dumpLine() << getBasicExprInfo(expr) << "\n";
}

void ASTDumper::visitCompoundStmt(CompoundStmt* node) {
  dumpLine() << getBasicStmtInfo(node) << '\n';
  indent();
  for (auto elem : node->getNodes())
    visit(elem);
  dedent();
}

void ASTDumper::visitConditionStmt(ConditionStmt* node) {
  dumpLine() << getBasicStmtInfo(node) << "\n";
  // Visit cond
  indent();
  visit(node->getCond());
  dedent();

  // Visit Then
  indent();
  visit(node->getThen());
  dedent();

  // If there's a else, visit it
  if (node->hasElse()) {
    indent();
    visit(node->getElse());
    dedent();
  }
}

void ASTDumper::visitWhileStmt(WhileStmt* node) {
  dumpLine() << getBasicStmtInfo(node) << "\n";
  // Visit cond
  indent();
  visit(node->getCond());
  dedent();

  // Visit body
  indent();
  visit(node->getBody());
  dedent();
}

void ASTDumper::visitReturnStmt(ReturnStmt* node) {
  dumpLine() << getBasicStmtInfo(node) << "\n";
  if (node->hasExpr()) {
    indent();
    visit(node->getExpr());
    dedent();
  }
}

void ASTDumper::visitUnitDecl(UnitDecl* node) {
  std::string fileInfo;
  if (auto file = node->getFileID())
    fileInfo = makeKeyPairDump("file", getFileNameOr(file, "unknown"));
  else
    fileInfo = makeKeyPairDump("file", "unknown");

  dumpLine() << getBasicDeclInfo(node) << " " << node->getIdentifier() << " "
    << fileInfo << " " << getDeclCtxtDump(node) << "\n";

  indent();
  for (auto decl : node->getDecls())
    visit(decl);
  dedent();
}

void ASTDumper::visitVarDecl(VarDecl* node) {
  dumpLine() << getValueDeclInfo(node) << "\n";
  if (node->hasInitExpr()) {
    indent(1);
    visit(node->getInitExpr());
    dedent(1);
  }
}

void ASTDumper::visitParamDecl(ParamDecl* node) {
  dumpLine() << getValueDeclInfo(node) << "\n";
}

void ASTDumper::visitFuncDecl(FuncDecl* node) {
  dumpLine() << getValueDeclInfo(node) << "\n";

  for (auto decl : *node->getParams()) {
    indent();
    visitParamDecl(decl);
    dedent();
  }

  // Visit the compound statement
  if (auto body = node->getBody()) {
    indent();
    visit(body);
    dedent();
  }
}

void ASTDumper::visit(Type type) {
  dumpLine() << toString(type);
}

bool ASTDumper::isDebug() const {
  return debug_;
}

std::string ASTDumper::toString(Type type) const {
  std::string typeStr = isDebug() ? type->toDebugString() : type->toString();
  return "'" + typeStr + "'";
}

std::string ASTDumper::toString(TypeLoc type) const {
  return toString(type.getType()) + ":" + toString(type.getSourceRange());
}

std::string ASTDumper::toString(SourceRange range) const {
  if (!hasSrcMgr())
    return "";
  return srcMgr_->getCompleteRange(range).toString(/*printFileName*/ false);
}

string_view 
ASTDumper::getFileNameOr(FileID file, string_view alternative) {
  if(!hasSrcMgr() || !file.isValid())
    return alternative;
  return srcMgr_->getFileName(file);
}

bool ASTDumper::hasSrcMgr() const {
  return (bool)srcMgr_;
}

std::ostream& ASTDumper::dumpLine(std::uint8_t num) {
  out << offset_ << getIndent(num);
  return out;
}

void ASTDumper::recalculateOffset() {
  offset_ = "";
  for (auto idx = offsetTabs_; idx > 0; idx--)
    offset_ += OFFSET_INDENT;
}

std::string ASTDumper::getIndent(const uint8_t& num) const {
  auto totalIndent = curIndent_ + num;
  if (totalIndent) {
    std::string rtr;
    for (auto k = totalIndent; k > 0; --k)
      rtr += INDENT;
    return rtr;
  }
  return "";
}

std::string ASTDumper::getStmtNodeName(Stmt* stmt) const {
  switch (stmt->getKind()) {
#define STMT(ID, PARENT) \
  case StmtKind::ID:     \
    return #ID;
#include "Fox/AST/StmtNodes.def"
    default:
      fox_unreachable("unknown node");
  }
}

std::string ASTDumper::getExprNodeName(Expr* expr) const {
  switch (expr->getKind()) {
#define EXPR(ID, PARENT) \
  case ExprKind::ID:     \
    return #ID;
#include "Fox/AST/ExprNodes.def"
    default:
      fox_unreachable("unknown node");
  }
}

std::string ASTDumper::getDeclNodeName(Decl* decl) const {
  switch (decl->getKind()) {
#define DECL(ID, PARENT) \
  case DeclKind::ID:     \
    return #ID;
#include "Fox/AST/DeclNodes.def"
    default:
      fox_unreachable("unknown node");
  }
}

std::string ASTDumper::getBasicStmtInfo(Stmt* stmt) const {
  std::ostringstream ss;
  ss << getStmtNodeName(stmt);
  if (isDebug())
    ss << " 0x" << (void*)stmt;
  ss << " " << getSourceRangeDump("range", stmt->getSourceRange());
  return ss.str();
}

std::string ASTDumper::getBasicExprInfo(Expr* expr) const {
  std::ostringstream ss;
  ss << getExprNodeName(expr);
  if (isDebug())
    ss << " 0x" << (void*)expr;
  if (auto ty = expr->getType()) {
    // Display "lvalue" before the type in non debug mode.
    // In debug mode, types are dumped in a debug form, so
    // the LValue is explicit.
    //
    //  e.g. for a string lvalue:
    //    non-debug will print: "lvalue 'string'"
    //    debug will print "'LValue(string)'"
    if(ty->is<LValueType>() && !isDebug())
      ss << " lvalue";
    ss << " " << toString(ty);
  }
  return ss.str();
}

std::string ASTDumper::getBasicDeclInfo(Decl* decl) const {
  std::ostringstream ss;
  std::string sourceRangeDump;
  ss << getDeclNodeName(decl)
     << " 0x" << (void*)decl
     << (decl->isLocal() ? " (local)" : "");

  ss << " " << getSourceRangeDump("range", decl->getSourceRange());
  return ss.str();
}

std::string ASTDumper::getValueDeclInfo(ValueDecl* decl) const {
  std::ostringstream ss;
  ss << getBasicDeclInfo(decl) << " ";

  std::string prefix;
  switch(decl->getKind()) {
    // For VarDecl, display the keyword
    case DeclKind::VarDecl:
    {
      VarDecl* var = cast<VarDecl>(decl);
      if(var->isLet())
        ss << "let ";
      else if(var->isVar())
        ss << "var ";
      else 
        fox_unreachable("unknown VarDecl keyword");
      break;
    }
    case DeclKind::ParamDecl:
    {
      ParamDecl* param = cast<ParamDecl>(decl);
      if(param->isMutable())
        ss << "mut ";
      break;
    }
    case DeclKind::FuncDecl:
      // Don't display anything for FuncDecls
      break;
    default:
      fox_unreachable("Unknown ValueDecl kind!");
  }

  ss << decl->getIdentifier() << " "
     << toString(decl->getValueType());
  return ss.str();
}

std::string ASTDumper::getOperatorDump(BinaryExpr* expr) const {
  std::ostringstream ss;
  ss << expr->getOpSign() << " (" << expr->getOpName() << ")";
  return ss.str();
}

std::string ASTDumper::getOperatorDump(UnaryExpr* expr) const {
  std::ostringstream ss;
  ss << expr->getOpSign() << " (" << expr->getOpName() << ")";
  return ss.str();
}

std::string ASTDumper::getDeclCtxtDump(DeclContext* dr) const {
  std::ostringstream ss;
  ss << "<DeclContext:0x" << (void*)dr;
  if (dr->hasParentDeclCtxt())
    ss << ", Parent:" << (void*)dr->getParentDeclCtxt();
  ss << ">";
  return ss.str();
}

std::string ASTDumper::getSourceLocDump(string_view label,
                                        SourceLoc sloc) const {
  if (sloc && hasSrcMgr()) {
    std::ostringstream ss;
    CompleteLoc cloc = srcMgr_->getCompleteLoc(sloc);
    ss  << cloc.line << ':' << cloc.column;
    return makeKeyPairDump(label, ss.str());
  } 
  return "";
}

std::string ASTDumper::getSourceRangeDump(string_view label,
  SourceRange range) const {
  if(hasSrcMgr())
    return makeKeyPairDump(label, toString(range));
  return "";
}

void ASTDumper::indent(std::uint8_t num) {
  curIndent_ += num;
}

void ASTDumper::dedent(std::uint8_t num) {
  if (curIndent_) {
    if (curIndent_ >= num)
      curIndent_ -= num;
    else
      curIndent_ = 0;
  }
}

// Dump methods
void Expr::dump() const {
  ASTDumper(std::cerr).dump(const_cast<Expr*>(this));
}

void Stmt::dump() const {
  ASTDumper(std::cerr).dump(const_cast<Stmt*>(this));
}

void Decl::dump() const {
  ASTDumper(std::cerr).dump(const_cast<Decl*>(this));
}

void TypeBase::dump() const {
  std::cerr << this->toDebugString() + '\n';
}

void Type::dump() const {
  if (ty_)
    ty_->dump();
  else
    std::cerr << "<nullptr>\n";
}

void DeclContext::dump() const {
  std::cerr << "DeclContext 0x" << (void*)this << "\n";
  if (hasDecls()) {
    std::cerr << "  --BEGIN DECLS DUMP--\n";
    for (auto decl : getDecls()) {
      std::cerr << "    0x" << (void*)decl << "\n";
    }
    std::cerr << "  --END DECLS DUMP--\n";
    if (lookupMap_) {
      std::cerr << "  --BEGIN LOOKUP TABLE DUMP--\n";
      for (auto entry : *lookupMap_) {
        ScopeInfo scope = entry.second.first;
        std::cerr << "    " << entry.first.getStr() << " -> {("
        // Dump the ScopeInfo
                  << +static_cast<typename std::underlying_type<ScopeInfo::Kind>::type>(scope.getKind())
                  << ", " << scope.getSourceRange() << "), 0x"
                  << entry.second.second << "}\n";
      }
      std::cerr << "  --END LOOKUP TABLE DUMP--\n";
    }
  }
  else 
    std::cerr << "  EMPTY\n";
}