## evaluate-expression-cpp

Been interested in making something along these lines for a while, and but always wanted something a little more versatile. Happy with this.

I wanna do a write up on how to implement unary operators in shunting-yard at some point!

----

### Basic usage:

Create an evaluator object

```
eval::evaluator_t evaluate;
```

Attach any operators/functions/constants you want to be available when parsing, in this exaple I've added all of the pre-configured operators/functions/constants

```
evaluate.add_operator(eval::operators::add);
evaluate.add_operator(eval::operators::subtract);
evaluate.add_operator(eval::operators::multiply);
evaluate.add_operator(eval::operators::divide);

evaluate.add_unary(eval::unary::plus);
evaluate.add_unary(eval::unary::minus);
evaluate.add_unary(eval::unary::percent);

evaluate.add_function(eval::functions::sqrt);
evaluate.add_function(eval::functions::pow);
evaluate.add_function(eval::functions::log);
evaluate.add_function(eval::functions::exp);

evaluate.add_constant(eval::constants::pi);
evaluate.add_constant(eval::constants::e);
```

We can then evaluate expressions by either calling the `evaluate` method, or preferrably the `operator()` overloaded method

```
std::cout << evaluate("(1+sqrt(5))/2") << std::endl;
```

----

Of course, the point here is customisation, you should be able add your own operators

### Custom operators

For example, let's imagine we're constructing the plus operator manually, with symbol '+'

First we need an operation function, which for operators takes the form `double(double, double)`

```c++
double my_add(const double a, const double b) { return a + b; }
```

We also need to define a second function, which checks if the parameters provided would cause the logic of the function to fail.

The is called the validator and has the form `bool(double, double)`.

Since addition is always valid, we can use the pre-defined validator `operators::always_valid`

But if we were creating the division operator, which can fail if we attempt to divide by zero, the validator should then be

```c++
bool div_validator(const double a, const double b) { return b != 0; }
```

We also need to choose our associativity and precedence, if this is new to you, here's the basics:

#### Associativity

An operator can be LEFT associative, for if our operator symbol is '+', then the expression

	a + b + c

is evaluated as

	(a + b) + c

however, if it was RIGHT associative, the expression would be

	a + (b + c)

All the basic operators '+', '-', '\*', and '/' are LEFT associative, so in general, you don't need to worry about it, but if you were to define '^' as the power operator, you'd have to define it as being RIGHT associative, because

    a ^ b ^ c

should evaluate as

    a ^ (b ^ c)

#### Precedence

Know your order of operations? Precedence is how programs handle order of operations.

Each operator has an associated precedence, represented usually a number, often an arbitrary number, but it's value matters when compared to the precedence of other operators. For example, suppose we had to expression

	a + b * c

If we have defined the precedence of the '+' operator to be 2, and the precedence of the '\*' operator to be 3, then because '\*' has GREATER precedence, we know to treat this expression as

	a + (b * c)

The default operators provided have precedence value 2 for '+' and '-', and 3 for '\*' and '/'.

We can define our own operators with the `operator_info_t` object

```c++
operator_info_t(char symbol, int precedence, const operation_t operation, associativity_t associativity = associativity_t::left, const validator_t validator = operators::always_valid);
```

### Custom unary operators

Unary operators are operators that apply to a single operand, so basically, the minus sign.

Like, like regular (binary) operators, to create a custom unary operator, we require a symbol, an operation function, and a validation function

The operation function takes the form `double(double)`, the validator function takes the form `bool(double)`

but we also specify an associativity. In the context of unary operators, the associativity tells if the operator acts on the object to its LEFT or its RIGHT.

For example, the minus operator acts on the object to it's RIGHT

	-10

But you could also define the factorial operator '!' which would act on the object to it's LEFT

	10!

To create a `unary_info_t` object:

```c++
unary_info_t(char symbol, const operation_t operation, associativity_t associativity = associativity_t::right, const validator_t validator = unary::always_valid);
```

### Custom functions

Functions require a name and a parameter count, as well as an operation and validation function, with types `double(const double*)` and `bool(const double*)` where the pointed value is an array of parameters.

`function_info_t` objects are constructed as such

```c++
function_info_t(const std::string& name, size_t param_count, const function_t function, const validator_t validator = functions::always_valid);
```

### Custom constants

Easy enough, just define a name and a value.

```c++
constant_info_t(const std::string &name, double value);
```