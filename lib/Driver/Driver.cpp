////------------------------------------------------------////
// This file is a part of The Moonshot Project.				
// See LICENSE.txt for license info.						
// File : Driver.cpp											
// Author : Pierre van Houtryve											
////------------------------------------------------------//// 
//			SEE HEADER FILE FOR MORE INFORMATION			
////------------------------------------------------------////

#include "Fox/Driver/Driver.hpp"

#include "Fox/Lexer/Lexer.hpp"
#include "Fox/Parser/Parser.hpp"
#include "Fox/AST/ASTDumper.hpp"
#include "Fox/AST/ASTContext.hpp"
#include "Fox/Sema/Sema.hpp"
#include "Fox/AST/Decl.hpp"
#include "Fox/Common/LLVM.hpp"
#include <iostream>
#include <chrono>
#include <fstream>

using namespace fox;

bool Driver::processFile(std::ostream& out, const std::string& filepath)
{
	SourceManager srcMgr;
	DiagnosticEngine dg(srcMgr);
	// Create a ASTContext
	ASTContext astCtxt;

	auto fid = srcMgr.loadFromFile(filepath);
	if (!fid)
	{
		out << "Could not open file \"" << filepath << "\"\n";
		return false;
	}
	auto t0 = std::chrono::high_resolution_clock::now();

	Lexer lex(dg, srcMgr, astCtxt);
	lex.lexFile(fid);

	auto t1 = std::chrono::high_resolution_clock::now();
	auto lex_micro = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
	auto lex_milli = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

	if (dg.getErrorsCount())
	{
		out << "Failed at lexing\n";
		return false;
	}

	Parser psr(dg,srcMgr, astCtxt,lex.getTokenVector());
	// Todo: extract the name of the file and use that instead of "TestUnit"
	auto unit = psr.parseUnit(fid, astCtxt.identifiers.getUniqueIdentifierInfo("TestUnit"), /* is main unit */ true);

	if (!unit)
	{
		out << "Failed at parsing.\n";
		return false;
	}

	out << "\nSuccess ! Dumping allocator:\n";
	astCtxt.getAllocator().dump(out);
	out << "\nDumping main unit:\n";

	auto t2 = std::chrono::high_resolution_clock::now();
	auto parse_micro = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	auto parse_milli = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
	
	ASTDumper dumper(srcMgr, std::cout, 1);
	dumper.visit(astCtxt.getMainUnit());

	auto t3 = std::chrono::high_resolution_clock::now();
	auto dump_micro = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
	auto dump_milli = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();

	out << "Fetching expr\n";

	// Dumb stuff for testing purposes, will be removed later
	FuncDecl* fdecl = cast<FuncDecl>(unit->getDecl(0));
	if (fdecl)
	{
		Expr* expr = fdecl->getBody()->getNode(0).get<Expr>();
		if (expr)
		{
			Sema sema;
			sema.checkExpr(expr);
			astCtxt.reset();
		}
	}

	auto t4 = std::chrono::high_resolution_clock::now();
	auto release_micro = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
	auto release_milli = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();


	out << "\nLexing time :\n\t" << lex_micro << " microseconds\n\t" << lex_milli << " milliseconds\n";
	out << "\nParsing time :\n\t" << parse_micro << " microseconds\n\t" << parse_milli << " milliseconds\n";
	out << "\nAST dump time :\n\t" << dump_micro << " microseconds\n\t" << dump_milli << " milliseconds\n";
	out << "\nAST release time :\n\t" << release_micro << " microseconds\n\t" << release_milli << " milliseconds\n";
	return true;
}