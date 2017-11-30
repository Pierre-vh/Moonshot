
/************************************************************
Author : Pierre van Houtryve
Contact :
e-mail : pierre.vanhoutryve@gmail.com

Description : See header

*************************************************************
MIT License

Copyright (c) 2017 Pierre van Houtryve

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*************************************************************/

#include "Dumper.h"

using namespace Moonshot;

Dumper::Dumper()
{
	std::cout << "Visitor \"Dumper\" Initialized. Dumping tree:" << std::endl;
}


Dumper::~Dumper()
{
}

FVal Dumper::visit(ASTExpr * node)
{
	std::cout << tabs() <<"ExpressionNode-> Operator:" << node->op_;
	if (node->totype_ != parse::types::NOCAST)
		std::cout << ",Casts to:" << node->totype_;
	std::cout << std::endl;
	if (node->left_)
	{
		std::cout << tabs() << char(192) << " Left child:" << tabs() << std::endl;
		tabcount += 1;
		node->left_->accept(this);
		tabcount -= 1;
	}
	if (node->right_)
	{
		std::cout << tabs() << char(192) << " Right child:" << tabs() << std::endl;
		tabcount += 1;
		node->right_->accept(this);
		tabcount -= 1;
	}
	return FVal();
}

FVal Dumper::visit(ASTValue * node)
{
	std::cout << tabs() << char(192) << "ExprValueNode -> " << dumpFVal(node->val_) << std::endl;
	return FVal();}

std::string Moonshot::Dumper::tabs() const
{
	std::string i;
	for (unsigned int k(0); k < tabcount; k++)
		i += '\t';
	return i;
}


