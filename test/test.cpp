#include <iostream>
#include "eval/evaluator.h"

int main(int argc, char const* argv[])
{
	eval::evaluator_t evaluate;

	evaluate.add_operator(eval::operators::add);
	evaluate.add_operator(eval::operators::subtract);
	evaluate.add_operator(eval::operators::multiply);
	evaluate.add_operator(eval::operators::divide);

	evaluate.add_unary(eval::unary::plus);
	evaluate.add_unary(eval::unary::minus);
	evaluate.add_unary(eval::unary::percent);

	evaluate.add_function(eval::functions::abs);
	evaluate.add_function(eval::functions::sqrt);
	evaluate.add_function(eval::functions::pow);
	evaluate.add_function(eval::functions::log);
	evaluate.add_function(eval::functions::exp);

	evaluate.add_constant(eval::constants::pi);
	evaluate.add_constant(eval::constants::e);

	evaluate.associate_pipe_with_implicit_function("abs");

	std::string line;
	while (std::cout << "> ", std::getline(std::cin, line) && !line.empty())
	{
		try
		{
			std::cout << evaluate(line) << std::endl;
		}
		catch (const eval::parse_exception& e)
		{
			// these occur if something went wrong duing the parsing of an expression
			// i.e. mismatch parentheses or just badly formatted expressions
			std::cout << "parse exception: " << e.what() << std::endl;
		}
		catch (const eval::evaluation_exception& e)
		{
			// these occur if the expression was valid, but a logical operation failed
			// i.e. attempted to divide by zero, or square root a negative number
			std::cout << "evaluation exception: " << e.what() << std::endl;
		}
	}

	return 0;
}