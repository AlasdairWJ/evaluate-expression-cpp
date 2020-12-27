#include "eval/evaluator.h"

#include <cmath>
#include <iostream>
#include <cctype>
#include <memory>
#include <stack>

namespace eval
{

// -----------------------------------------------------------------------------

// OPERATORS

operator_info_t::operator_info_t(const char symbol, const int precedence, const operator_info_t::operation_t operation,
	const associativity_t associativity, const operator_info_t::validator_t validator)
	: m_symbol(symbol), m_precedence(precedence), m_operation(operation)
	, m_associativity(associativity), m_validator(validator)
{
}

namespace operators
{

bool always_valid(const double a, const double b) { return true; }

namespace
{
bool b_ne_zero(const double a, const double b) { return b != 0; }

double _add(const double a, const double b) { return a + b; }
double _subtract(const double a, const double b) { return a - b; }
double _multiply(const double a, const double b) { return a * b; }
double _divide(const double a, const double b) { return a / b; }
}

const operator_info_t add('+', 2, _add);
const operator_info_t subtract('-', 2, _subtract);
const operator_info_t multiply('*', 3, _multiply);
const operator_info_t divide('/', 3, _divide, associativity_t::left, b_ne_zero);
}

// -----------------------------------------------------------------------------

// FUNCTIONS

function_info_t::function_info_t(const std::string& name, const size_t param_count,
	const function_info_t::function_t function, const function_info_t::validator_t validator)
	: m_name(name), m_param_count(param_count)
	, m_function(function), m_validator(validator)
{
}

namespace functions
{

bool always_valid(const double* args) { return true; }

namespace
{
bool arg0_gt_zero(const double* args) { return args[0] > 0; }
bool arg0_ge_zero(const double* args) { return args[0] > 0; }

double _log(const double *args) { return std::log(args[0]); }
double _exp(const double *args) { return std::exp(args[0]); }
double _sqrt(const double *args) { return std::sqrt(args[0]); }
double _pow(const double *args) { return std::pow(args[0], args[1]); }
bool pow_validator(const double *args) { return args[1] >= 0 || fmod(args[1], 1.0) == 0; }
}

const function_info_t sqrt("sqrt", 1, _sqrt, arg0_ge_zero);
const function_info_t exp("exp", 1, _exp, always_valid);
const function_info_t log("log", 1, _log, arg0_gt_zero);
const function_info_t pow("pow", 2, _pow, pow_validator);

}

// -----------------------------------------------------------------------------

// CONSTANTS

constant_info_t::constant_info_t(const std::string& name, double value)
	: m_name(name), m_value(value)
{
}

namespace constants
{
const constant_info_t pi("pi", 3.141592653589793);
const constant_info_t e("e", 2.718281828459045);
}

// -----------------------------------------------------------------------------

// UNARY

unary_info_t::unary_info_t(char symbol, const unary_info_t::operation_t operation, const associativity_t associativity, const unary_info_t::validator_t validator)
	: m_symbol(symbol), m_associativity(associativity), m_operation(operation), m_validator(validator)
{
}

namespace unary
{
bool always_valid(const double x) { return true; }

namespace
{
double _plus(const double x) { return x; }
double _minus(const double x) { return -x; }
double _percent(const double x) { return x / 100.0; }
}

const unary_info_t plus('+', _plus);
const unary_info_t minus('-', _minus);
const unary_info_t percent('%', _percent, associativity_t::left);
}

// -----------------------------------------------------------------------------

// meh
template <typename... Args>
std::string lazy_format(const char* fmt, Args... args)
{	
	char buffer[128];
	sprintf_s(buffer, fmt, args...);
	return buffer;
}

// -----------------------------------------------------------------------------

bool evaluator_t::read_token(const std::string& line, size_t& position, token_t& token, const bool expecting_identifier) const
{
	if (expecting_identifier)
	{
		if (isalpha(line[position]))
		{
			size_t identifier_length = 1;

			while (isalnum(line[position + identifier_length]))
				identifier_length++;

			const std::string identifier = line.substr(position, identifier_length);

			const auto function_it = m_function_name_map.find(identifier);
			if (function_it != m_function_name_map.end())
			{
				token.m_type = token_type::FUNCTION;
				token.m_id = function_it->second;
				position += identifier_length;
				return true;
			}

			const auto constant_it = m_constant_name_map.find(identifier);
			if (constant_it != m_constant_name_map.end())
			{
				token.m_type = token_type::CONSTANT;
				token.m_id = constant_it->second;
				position += identifier_length;
				return true;
			}
		}

		const auto unary_it = m_unary_map.find(line[position]);
		if (unary_it != m_unary_map.end() && unary_it->second.m_associativity == associativity_t::right)
		{
			token.m_type = token_type::UNARY;
			token.m_symbol = line[position++];
			return true;
		}

		if (isdigit(line[position]))
		{
			token.m_type = token_type::NUMBER;
			int n;
			sscanf_s(&line[position], "%lf%n", &token.m_value, &n);
			position += n;
			return true;
		}
	}
	else
	{
		if (line[position] == ')')
		{
			token.m_type = token_type::RIGHT_PAREN;
			token.m_symbol = line[position++];
			return true;
		}

		if (line[position] == ',')
		{
			token.m_type = token_type::COMMA;
			token.m_symbol = line[position++];
			return true;
		}

		if (m_operator_map.find(line[position]) != m_operator_map.end())
		{
			token.m_type = token_type::OPERATOR;
			token.m_symbol = line[position++];
			return true;
		}

		const auto unary_it = m_unary_map.find(line[position]);
		if (unary_it != m_unary_map.end() && unary_it->second.m_associativity == associativity_t::left)
		{
			token.m_type = token_type::UNARY;
			token.m_symbol = line[position++];
			return true;
		}
	}

	return false;
}

std::list<token_t> evaluator_t::tokenise(const std::string& line) const
{
	std::list<token_t> output;

	bool expecting_identifier = true;
	bool expecting_left_paren = false;

	for (size_t position = 0; position < line.size();)
	{
		if (isspace(line[position]))
		{
			position++;
			continue;
		}

		token_t token;

		if (line[position] == '(')
		{
			token.m_type = token_type::LEFT_PAREN;
			token.m_symbol = line[position++];

			expecting_left_paren = false;
		}
		else
		{
			if (expecting_left_paren)
				throw parse_exception("no left paren immediately after function token");

			if (!read_token(line, position, token, expecting_identifier))
				throw parse_exception(lazy_format("failed to read token at position %llu", position));
		}


		output.push_back(token);

		switch (token.m_type)
		{
		case token_type::FUNCTION:
			expecting_left_paren = true;
		case token_type::LEFT_PAREN:
		case token_type::COMMA:
		case token_type::OPERATOR:
			expecting_identifier = true;
			break;
		case token_type::UNARY:
			expecting_identifier = m_unary_map.find(token.m_symbol)->second.m_associativity == associativity_t::right;
			break;
		case token_type::RIGHT_PAREN:
		case token_type::NUMBER:
		case token_type::CONSTANT:
		default:
			expecting_identifier = false;
			break;
		}
	}

	if (expecting_identifier)
		throw parse_exception("expecting terminating identifier");

	return output;
}

// -----------------------------------------------------------------------------

void evaluator_t::print_tokens(const std::list<token_t>& tokens) const
{
	for (const token_t& token : tokens)
	{
		switch (token.m_type)
		{
		case token_type::LEFT_PAREN:
		case token_type::RIGHT_PAREN:
		case token_type::OPERATOR:
		case token_type::UNARY:
		case token_type::COMMA:
			std::cout << token.m_symbol;
			break;

		case token_type::NUMBER:
			std::cout << token.m_value;
			break;

		case token_type::FUNCTION:
			std::cout << m_functions[token.m_id].m_name;
			break;

		case token_type::CONSTANT:
			std::cout << m_constants[token.m_id].m_name;
			break;

		default:
			std::cout << "[?]";
			break;
		}
		putchar(' ');
	}
	putchar('\n');
}

// -----------------------------------------------------------------------------

std::list<token_t> evaluator_t::to_postfix(const std::list<token_t>& infix_tokens) const
{
	std::stack<token_t> stack;

	// holds the number of arguments for the function currently being evaluated
	// when we reach a "," we meed to treat that as an end-of-line, popping operators
	std::stack<size_t> paren_stack;

	std::list<token_t> postfix_tokens;

	for (const auto& token : infix_tokens)
	{
		switch (token.m_type)
		{
		case token_type::NUMBER:
		case token_type::CONSTANT:
			postfix_tokens.push_back(token);

			while (!stack.empty() && stack.top().m_type == token_type::UNARY)
			{
				postfix_tokens.push_back(stack.top());
				stack.pop();
			}

			break;

		case token_type::UNARY: {
			const unary_info_t& info = m_unary_map.find(token.m_symbol)->second;
			if (info.m_associativity == associativity_t::right)
			{
				stack.push(token);
			}
			else // if (info.m_associativity == associativity_t::left)
			{
				postfix_tokens.push_back(token);
			}
			break;
		}
		case token_type::FUNCTION: {
			stack.push(token); // actual logic handled by followed '('
			break;
		}
		case token_type::OPERATOR: {
			const auto& info = m_operator_map.find(token.m_symbol)->second;
			decltype(m_operator_map.begin()) it;
			while (!stack.empty() && stack.top().m_type == token_type::OPERATOR &&
				(it = m_operator_map.find(stack.top().m_symbol), it->second.m_precedence > info.m_precedence ||
					(it->second.m_precedence == info.m_precedence && info.m_associativity == associativity_t::left)))
			{
				postfix_tokens.push_back(stack.top());
				stack.pop();
			}
			stack.push(token);
			break;
		}
		case token_type::COMMA: {
			if (paren_stack.empty())
				throw parse_exception("bad comma");

			size_t& top = paren_stack.top();

			if (top < 1)
				throw parse_exception("bad comma OR too many args to function");

			top--;

			while (!stack.empty() && stack.top().m_type == token_type::OPERATOR)
			{
				postfix_tokens.push_back(stack.top());
				stack.pop();
			}

			break;
		}
		case token_type::LEFT_PAREN:
			if (!stack.empty() && stack.top().m_type == token_type::FUNCTION)
			{
				paren_stack.push(m_functions[stack.top().m_id].m_param_count);
			}
			else
			{
				paren_stack.push(1llu);
			}
			stack.push(token);
			break;

		case token_type::RIGHT_PAREN:
			if (paren_stack.empty())
				throw parse_exception("got close paren, but no open?");

			if (paren_stack.top() != 1) // count is decreased each arg, so final should be 1
				throw parse_exception("not enough args?" + std::to_string(paren_stack.top()));

			paren_stack.pop();

			while (!stack.empty() && stack.top().m_type == token_type::OPERATOR)
			{
				postfix_tokens.push_back(stack.top());
				stack.pop();
			}

			if (stack.empty() || stack.top().m_type != token_type::LEFT_PAREN)
				throw parse_exception("mismatched parentheses, closing unmatched parenthesis");

			stack.pop();

			if (!stack.empty() && stack.top().m_type == token_type::FUNCTION)
			{
				postfix_tokens.push_back(stack.top());
				stack.pop();
			}

			while (!stack.empty() && stack.top().m_type == token_type::UNARY)
			{
				postfix_tokens.push_back(stack.top());
				stack.pop();
			}

			break;
		}
	}

	while (!stack.empty())
	{
		if (stack.top().m_type == token_type::LEFT_PAREN)
			throw parse_exception("mismatched parentheses, unclosed parenthesis");

		postfix_tokens.push_back(stack.top());
		stack.pop();
	}

	return postfix_tokens;
}

// -----------------------------------------------------------------------------

double evaluator_t::evaluate(std::list<token_t> postfix_tokens) const
{
	auto get_token_value = [&](const token_t& token)
		{
			switch (token.m_type)
			{
			case token_type::NUMBER:
				return token.m_value;
			case token_type::CONSTANT:
				return m_constants[token.m_id].m_value;
			default:
				// shouldn't happen, unless someone's been fiddling with the tokens...
				throw evaluation_exception("trying to get value from non-const non-number token");
			}
		};

	for (auto it = postfix_tokens.begin(); it != postfix_tokens.end(); ++it)
	{
		switch (it->m_type)
		{
		case token_type::UNARY:
		{
			const char op = it->m_symbol;
			const auto& unary_info = m_unary_map.find(op)->second;

			const double x = get_token_value(*(--it));

			it = postfix_tokens.erase(it);

			if (!unary_info.m_validator(x))
				throw evaluation_exception(lazy_format("unary validator failed (%c)", op));

			const double result = unary_info.m_operation(x);

			it->m_type = token_type::NUMBER;
			it->m_value = result;
			break;
		}
		case token_type::OPERATOR:
		{
			const char op = it->m_symbol;
			const auto& operator_info = m_operator_map.find(op)->second;

			const double b = get_token_value(*(--it));
			const double a = get_token_value(*(--it));

			it = postfix_tokens.erase(it);
			it = postfix_tokens.erase(it);

			if (!operator_info.m_validator(a, b))
				throw evaluation_exception(lazy_format("operator validator failed (%c)", op));

			const double result = operator_info.m_operation(a, b);

			it->m_type = token_type::NUMBER;
			it->m_value = result;
			break;
		}
		case token_type::FUNCTION:
		{
			const size_t function_id = it->m_id;

			const auto& function_info = m_functions[function_id];

			auto args = std::make_unique<double[]>(function_info.m_param_count);

			for (size_t i = function_info.m_param_count; i > 0;)
				args[--i] = get_token_value(*(--it));

			for (size_t i = 0; i < function_info.m_param_count; i++)
				it = postfix_tokens.erase(it);

			if (!function_info.m_validator(args.get()))
				throw evaluation_exception(lazy_format("function validator failed (%s)", function_info.m_name.c_str()));

			const double result = function_info.m_function(args.get());

			it->m_type = token_type::NUMBER;
			it->m_value = result;
			break;
		}
		default:
			break;
		}
	}

	return get_token_value(postfix_tokens.front());
}

// -----------------------------------------------------------------------------

std::list<token_t> evaluator_t::parse(const std::string& expression) const
{
	return to_postfix(tokenise(expression));
}

double evaluator_t::evaluate(const std::string& expression) const
{
	return evaluate(parse(expression));
}

double evaluator_t::operator()(const std::string& expression) const
{
	return evaluate(expression);
}

// -----------------------------------------------------------------------------

evaluator_t &evaluator_t::add_constant(const constant_info_t& info)
{
	const size_t id = m_constants.size();
	m_constants.push_back(info);
	m_constant_name_map.emplace(info.m_name, id);
	return *this;
}


evaluator_t& evaluator_t::add_operator(const operator_info_t& info)
{
	m_operator_map.emplace(info.m_symbol, info);
	return *this;
}

evaluator_t& evaluator_t::add_unary(const unary_info_t& info)
{
	m_unary_map.emplace(info.m_symbol, info);
	return *this;
}

evaluator_t &evaluator_t::add_function(const function_info_t& info)
{
	const size_t id = m_functions.size();
	m_functions.push_back(info);
	m_function_name_map.emplace(info.m_name, id);
	return *this;
}

// -----------------------------------------------------------------------------

parse_exception::parse_exception(const std::string& error_message)
	: m_error_message(error_message)
{
}

const char* parse_exception::what() const throw ()
{
	return m_error_message.c_str();
}

// -----------------------------------------------------------------------------

evaluation_exception::evaluation_exception(const std::string& error_message)
	: m_error_message(error_message)
{
}

const char* evaluation_exception::what() const throw ()
{
	return m_error_message.c_str();
}

// -----------------------------------------------------------------------------


}