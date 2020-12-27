#pragma once

#include <string>
#include <map>
#include <vector>
#include <list>
#include <exception>

namespace eval
{

enum class associativity_t { left, right };

// -----------------------------------------------------------------------------

struct constant_info_t
{
	const std::string m_name;
	const double m_value;

	constant_info_t(const std::string &name, double value);
};

namespace constants
{
extern const constant_info_t pi;
extern const constant_info_t e;
}

// -----------------------------------------------------------------------------

namespace operators
{
bool always_valid(const double a, const double b);
}

struct operator_info_t
{
	typedef double(*operation_t)(double, double);
	typedef bool(*validator_t)(double, double);

	const char m_symbol;
	const int m_precedence;
	const operation_t m_operation;
	const associativity_t m_associativity;
	const validator_t m_validator;

	operator_info_t(char symbol, int precedence, const operation_t operation,
		associativity_t associativity = associativity_t::left, const validator_t validator = operators::always_valid);
};

namespace operators
{
extern const operator_info_t add;
extern const operator_info_t subtract;
extern const operator_info_t multiply;
extern const operator_info_t divide;
}

// -----------------------------------------------------------------------------

namespace unary
{
bool always_valid(double x);
}

struct unary_info_t
{
	typedef double(*operation_t)(double);
	typedef bool(*validator_t)(double);

	const char m_symbol;
	const associativity_t m_associativity;
	const operation_t m_operation;
	const validator_t m_validator;

	unary_info_t(char symbol, const operation_t operation, associativity_t associativity = associativity_t::right, const validator_t validator = unary::always_valid);
};

namespace unary
{
extern const unary_info_t plus;
extern const unary_info_t minus;
extern const unary_info_t percent;
}

// -----------------------------------------------------------------------------

namespace functions
{
bool always_valid(const double* args);
}

struct function_info_t
{
	typedef double(*function_t)(const double*);
	typedef bool(*validator_t)(const double*);

	const std::string m_name;
	const size_t m_param_count;
	const function_t m_function;
	const validator_t m_validator;

	function_info_t(const std::string& name, size_t param_count, const function_t function, const validator_t validator = functions::always_valid);
};

namespace functions
{
extern const function_info_t sqrt;
extern const function_info_t exp;
extern const function_info_t log;
extern const function_info_t pow;
}

// -----------------------------------------------------------------------------

enum class token_type
{
	LEFT_PAREN,
	RIGHT_PAREN,
	COMMA,
	NUMBER,
	CONSTANT,
	OPERATOR,
	UNARY,
	FUNCTION
};

struct token_t
{
	token_type m_type;
	union
	{
		char m_symbol;
		size_t m_id;
		double m_value;
	};
};

struct evaluator_t
{
	// -------------------------------------------------------------------------

	std::list<token_t> parse(const std::string &expression) const;
	double evaluate(std::list<token_t> postfix_tokens) const;
	
	// -------------------------------------------------------------------------

	double evaluate(const std::string &expression) const;
	double operator()(const std::string &expression) const;

	// -------------------------------------------------------------------------

	void print_tokens(const std::list<token_t>& tokens) const;

	// -------------------------------------------------------------------------

	evaluator_t& add_constant(const constant_info_t& info);
	evaluator_t& add_operator(const operator_info_t& info);
	evaluator_t& add_unary(const unary_info_t &info);
	evaluator_t& add_function(const function_info_t &info);

	// -------------------------------------------------------------------------

private:
	std::list<token_t> tokenise(const std::string& expression) const;
	std::list<token_t> to_postfix(const std::list<token_t>& infix_tokens) const;

	bool read_token(const std::string& line, size_t& position, token_t& token, bool expecting_identifier) const;
	
	// -------------------------------------------------------------------------

	std::vector<constant_info_t> m_constants;
	std::map<std::string, size_t> m_constant_name_map;

	std::map<char, operator_info_t> m_operator_map;

	std::map<char, unary_info_t> m_unary_map;

	std::vector<function_info_t> m_functions;
	std::map<std::string, size_t> m_function_name_map;
};

// -----------------------------------------------------------------------------

struct parse_exception : public std::exception
{
	parse_exception(const std::string& error_message);

	const char* what () const throw ();

private:
	const std::string m_error_message;
};

// -----------------------------------------------------------------------------

struct evaluation_exception : public std::exception
{
	evaluation_exception(const std::string& error_message);

	const char* what () const throw ();

private:
	const std::string m_error_message;
};
}