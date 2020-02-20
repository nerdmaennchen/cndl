# QRQMA
qrqma is a C++ implementaion of [Jinja](https://jinja.palletsprojects.com/) templates that tries to go as far as possible to reach Jinja's flexibility while retaining as much C++ mentality as possible.
Please bear in mind that qrqma is not a 1to1 drop in replacement for Jinja since some of Jinja's features simply cannot be realized in C++. 
Also qrqma is still missing some features.

Most of this documentation is taken from the [Jinja documentation](https://jinja.palletsprojects.com/).
If you love Jinja but miss using it from C++, you might love qrqma as well.

The motivation behind qrqma is to bring the ease of dynamic content generation from Jinja (or django) to the world of C++.
Below is an analogous minimal template example to the [one in the Jinja documentation](https://jinja.palletsprojects.com/en/2.10.x/templates/#synopsis).
The biggest difference to the Jinja example is that within a qrqma template you cannot access members of an object.
Therefore the functions ``href`` and ``caption`` need to be provided.
The reason for this is C++'s lack of runtime reflection.
qrqma is an attempt to get a template engine as far as possible in C++ (with type safety where it is possible and compiled templates for quick rendering!).

~~~C++
#include <iostream>
#include "qrqma/template.h"

int main() {
    auto templateStr = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <title>My Webpage</title>
</head>
<body>
    <ul id="navigation">
    {% for item in navigation %}
        <li><a href="{{ href(item) }}">{{ caption(item) }}</a></li>
    {% endfor %}
    </ul>

    <h1>My Webpage</h1>
    {{ a_variable }}

    {# a comment #}
</body>
</html>)";
    struct NavItem { std::string href, caption; };
    qrqma::Template rendering{
        templateStr,
        {
            {"navigation", qrqma::symbol::List{{NavItem{"Home", "index.html"}, NavItem{"Blog", "blog.html"}}} },
            {"href", qrqma::symbol::Function{[](NavItem const& ni) {
                return ni.href;
            }}},
            {"caption", qrqma::symbol::Function{[](NavItem const& ni) {
                return ni.caption;
            }}},
            {"a_variable", std::string{"qrqma is awesome!"}}
        }
    };
    std::cout << rendering() << std::endl;
    return 0;
}
~~~

More examples can be visited in the demo branch!
Look for [demo.cpp](https://github.com/nerdmaennchen/qrqma/blob/demo/src/demo.cpp) and [qrqma_test.cpp](https://github.com/nerdmaennchen/qrqma/blob/demo/src/qrqma_test.cpp).

# Missing in qrqma

- Filters
- tests (only the "is [test]" part); if statements are implemented
- Line statements
- Super blocks
- Macros (and Call)
- Loop-else blocks
- namespace objects
- inline if
  
# Getting Started

To use qrqma in you C++ project the easiest way is to simply copy the qrqma sources into your project source directory and you're set.

## Statements

Jinja allows for four types of statements, whereas qrqma only allows for three (line statements are not implemented)

- ``{% ... %}`` for (control) Statements
- ``{{ ... }}`` for Expressions to print to the template output
- ``{# ... #}`` for Comments not included in the template output

## Variables

Template variables are passed to the template renderer or defined inside the template.
There are two stages when you can pass variables to the renderer:
- during the Template's construction time  
  Variables passed this way will be considered constant; if the template compiler can deduce that a variable/expression substitution will yield a constant it will reduce the expression and possibly generate static-only output.
  This is what makes qrqma so fast!
- during the Template's rendering time
  Variables passed here will be rendered the very same way as in Jinja.
  Except for variables that were already specified during compilation. Pre-rendered expressions/variables cannot be overridden during rendering time.

Attributes and member functions of variables cannot be (directly) accessed from within a qrqma template. However, a function to provide an accessor can easily be passed to the renderer.
Elements of variables of type ``symbols::Map`` (which implies a map from ``std::string``s as keys to ``std::any``s) can be accessed like so:

~~~
{{ foo['bar'] }}
~~~

If a variable does not exist during compile time, expressions involving this variable will be evaluated during render-time.
If the variable is not defined during render time an exception will be thrown when the variable is used inside a calculation.
An undefined variable will render to the empty string when used in a ``{{}}`` print-expression.

## Functions

You can pass a ``symbols::Function`` to the template and enable calls within the template to the passed function.
Like so:
~~~C++
auto templateStr = R"(
{% set my_list = make_list(5) -%}
<ul>
{%- for i in my_list %}
<li>{{ title(i) }}</li>
{%- endfor %}
</ul>
)";
struct ListElement {
    std::string title;
};
qrqma::Template rendering{
    content,
    {
        // to play with qrqma add symbols here
        {"make_list", qrqma::symbol::Function{[](int numElements) {
            qrqma::symbol::List l;
            for (int i{}; i < numElements; ++i) {
                l.emplace_back(ListElement{"title: " + std::to_string(i)});
            }
            return l;
        }}},
        {"title", qrqma::symbol::Function{[](ListElement const& le) {
            return le.title;
        }}},
    }
};
std::cout << rendering() << std::endl;
~~~

This will generate the following output:

~~~html
<ul>
<li>title: 0</li>
<li>title: 1</li>
<li>title: 2</li>
<li>title: 3</li>
<li>title: 4</li>
</ul>
~~~

Note that there is no member access operator within qrqma.
Therefore, you need to pass a function that wraps this access.

In general, function calls are performed in a type-safe manner!
If the function you've supplied to the template has a single argument of type ``int``, it will only be invoked with an ``int``.
Qrqma tries to convert supplied types as good as possible but cannot do more automatic casts that C++ could. 

If the template does something with a variable where no type conversion is known (e.g. pass a variable of type ``bool`` to a function that expects a custom type) an exception will be thrown.
Also an exception will be thrown if an undefined function is called during render-time.

## Filters

qrqma does not implement filters as Jinja does.
However, filters can easily be created as simple function calls.

## Tests

qrqma supports ``if`` statements like the following:
~~~
{% if foo == 2 and True != False or "string" %}...{% else %}...{% endif %}
~~~
The else block is optional.
The semantics behind the ``if``-head are very similar to the semantics in python:
A nonempty string is considered True, a number different from 0 also, etc.

## Comments

To comment out a part of the template simply wrap the comment in a ``{# comment #}`` tag.

## Whitespace Control

By default qrqma does not treat any whitespaces differently.
Everything that is part of the output, will be output.
However, you can use a single dash (``-``) at the beginning or end of a control statement to suppress all whitespaces leading or trailing to the statement.
It's best demonstrated by an example:

~~~html
<p>text</p>
{% if True %}
foobar
{% endif %}
<p>mote text</p>
~~~
will be rendered to:
~~~html
<p>text</p>

foobar

<p>mote text</p>
~~~

With whitespace control you can suppress the output of the empty lines like so:
~~~html
<p>text</p>
{% if True -%}
foobar
{%- endif -%}
<p>mote text</p>
~~~
will be rendered to:
~~~html
<p>text</p>
foobar<p>mote text</p>
~~~

###Note:
You must not add whitespace between the tag and the minus sign.

valid:
~~~
{%- if foo -%}...{% endif %}
~~~
invalid:
~~~
{% - if foo - %}...{% endif %}
~~~

## Escaping

It is sometimes desirable – even necessary – to have qrqma ignore parts it would otherwise handle as variables or blocks. For example, if, with the default syntax, you want to use {{ as a raw string in a template and not start a variable, you have to use a trick.

The easiest way to output a literal variable delimiter ({{) is by using a variable expression:

~~~html
{{ '{{' }}
~~~

For bigger sections, it makes sense to mark a block raw. For example, to include example Jinja syntax in a template, you can use this snippet:

~~~html
{% raw %}
    <ul>
    {% for item in seq %}
        <li>{{ item }}</li>
    {% endfor %}
    </ul>
{% endraw %}
~~~

## Template Inheritance

The most powerfull feature of Jinja is [inheritance](https://jinja.palletsprojects.com/en/2.10.x/templates/#template-inheritance).
qrqma can do the same thing.
The rest of this section is alsmost word-by-word taken from the Jinja documentation as the template inheritance methods are working **very** similarly.
If you like jinja's template inheritance, you should feel right at home with qrqma.

Template inheritance allows you to build a base “skeleton” template that contains all the common elements of your site and defines blocks that child templates can override.

Sounds complicated but is very basic. It’s easiest to understand it by starting with an example.

### Base Template
This template, which we’ll call base.html, defines a simple HTML skeleton document that you might use for a simple two-column page. It’s the job of “child” templates to fill the empty blocks with content:

~~~html
<!DOCTYPE html>
<html lang="en">
<head>
    {% block head %}
    <link rel="stylesheet" href="style.css" />
    <title>{% block title %}{% endblock %} - My Webpage</title>
    {% endblock %}
</head>
<body>
    <div id="content">{% block content %}{% endblock %}</div>
    <div id="footer">
        {% block footer %}
        &copy; Copyright 2008 by <a href="http://domain.invalid/">you</a>.
        {% endblock %}
    </div>
</body>
</html>
~~~

In this example, the {% block %} tags define four blocks that child templates can fill in. All the block tag does is tell the template engine that a child template may override those placeholders in the template.

### Child Template
A child template might look like this:

~~~html
{% extends "base.html" %}
{% block title %}Index{% endblock %}
{% block head %}
    {{ super() }}
    <style type="text/css">
        .important { color: #336699; }
    </style>
{% endblock %}
{% block content %}
    <h1>Index</h1>
    <p class="important">
      Welcome to my awesome homepage.
    </p>
{% endblock %}
~~~

The ``{% extends %}`` tag is the key here. It tells the template engine that this template “extends” another template. When the template system evaluates this template, it first locates the parent. The extends tag should be the first tag in the template. Everything before it is printed out normally and may cause confusion. For details about this behavior and how to take advantage of it, see Null-Master Fallback. Also a block will always be filled in regardless of whether the surrounding condition is evaluated to be true or false.

The filename of the template depends on the template loader. For example, the ``defaultLoader`` allows you to access other templates by giving the filename. You can access templates in subdirectories with a slash:
~~~
{% extends "layout/default.html" %}
~~~

But this behavior can depend on the application embedding qrqma. Note that since the child template doesn’t define the footer block, the value from the parent template is used instead.

You shouldn't define multiple ``{% block %}`` tags with the same name in the same child template. This is discouraged because a block tag works in “both” directions. That is, a block tag doesn’t just provide a placeholder to fill - it also defines the content that fills the placeholder in the parent. If there were two same-named ``{% block %}`` tags in a child template, that template’s parent wouldn’t know which one of the blocks’ content to use.
However, in base templates each ``{% block %}`` will get the content of the child template's content.

## Named Block End-Tags

qrqma allows you to put the name of the block after the end tag for better readability:

~~~
{% block sidebar %}
    {% block inner_sidebar %}
        ...
    {% endblock inner_sidebar %}
{% endblock sidebar %}
~~~

There is no functionality behind the end-tags.
Technically you can write whatever you like in there.

## Block Nesting and Scope

Blocks can be nested for more complex layouts.
However, blocks may not access variables from outer scopes:

base template:
~~~
{% for item in seq %}
    <li>{% block loop_item %}{% endblock %}</li>
{% endfor %}
~~~

child template:
~~~
{% block loop_item %}{{ item }}{% endblock %}
~~~

The template parser would try to generate the output of the child template's ``loop_item`` block but fail there as ``item`` is not defined in the block's scope.

# List of Control Structures

A control structure refers to all those things that control the flow of a program - conditionals (i.e. if/elif/else), for-loops, as well as things like macros and blocks. With the default syntax, control structures appear inside ``{% ... %}`` blocks.

## For

Loop over each item in a sequence. For example, to display a list of users provided in a variable called users:

~~~html
<h1>Members</h1>
<ul>
{% for user in users %}
  <li>{{ user }}</li>
{% endfor %}
</ul>
~~~

As variables in templates retain their object properties, it is possible to iterate over containers like dict.
Note that the pythonic call to ``.items()`` is not necessary in qrqma.
Either the key-value-pairs are broadcasted into two variables or the key is broadcasted into the single loop variable

~~~html
<dl>
{% for key, value in my_dict %}
    <dt>{{ key }}</dt>
    <dd>{{ value }}</dd>
{% endfor %}
</dl>
~~~

Or similarly using a single loop variable:
~~~html
<dl>
{% for key in my_dict %}
    <dt>{{ key }}</dt>
    <dd>{{ my_dict[key] }}</dd>
{% endfor %}
</dl>
~~~

Note, however, that C++ ``std::map``s are **ordered**.

Inside of a for-loop block, you can access some special variables:

| Variable       | Description                                                                             |
| -------------- | --------------------------------------------------------------------------------------- |
| loop.index     | The current iteration of the loop. (1 indexed)                                          |
| loop.index0    | The current iteration of the loop. (0 indexed)                                          |
| loop.revindex  | The number of iterations from the end of the loop (1 indexed)                           |
| loop.revindex0 | The number of iterations from the end of the loop (0 indexed)                           |
| loop.first     | True if first iteration.                                                                |
| loop.last      | True if last iteration.                                                                 |
| loop.length    | The number of items in the sequence.                                                    |
| loop.depth     | Indicates how deep in a recursive loop the rendering currently is. Starts at level 1    |
| loop.depth0    | Indicates how deep in a recursive loop the rendering currently is. Starts at level 0    |
| loop.previtem  | The item from the previous iteration of the loop. Undefined during the first iteration. |
| loop.nextitem  | The item from the following iteration of the loop. Undefined during the last iteration. |

The loop variable always refers to the closest (innermost) loop.
If we have more than one level of loops, we can rebind the loop variables by writing ``{% set outer_loop.index = loop.index %}`` after the loop that we want to use recursively.
Then, we can call it using ``{{ outer_loop.index }}``.
As qrqma does not support member access, variables are allowed to have a '.' in their names. 
In the example the variable ``outer_loop.index`` is a single variable with a slightly confusing name.
This was chosen to give a as close-to-Jinja experience as possible.

In contrast fo Jinja, variables declared within loops _will_ also be visible outside of the loop's scope.
This reflects python's behavior of scopes.
Note that the special variables from within a loop will _not_ be visible outside the scope.

## If

The if statement in qrqma is comparable with the Jinja if statement. In the simplest form, you can use it to test if a variable is not empty and not false:

~~~html
{% if users %}
<ul>
{% for user in users %}
    <li>{{ user }}</li>
{% endfor %}
</ul>
{% endif %}
~~~

For multiple branches, elif and else can be used like in Python. You can use more complex Expressions there, too:

~~~
{% if kenny.sick %}
    Kenny is sick.
{% elif kenny.dead %}
    You killed Kenny!  You bastard!!!
{% else %}
    Kenny looks okay --- so far
{% endif %}
~~~

## Assignments

Inside control blocks, you can also assign values to variables. Assignments at top  level that are specified _before_ the ``{% extends ... %}`` keyword are exported from the template and can be used within parent templates.

Assignments use the set tag:
~~~
{% set navigation_map = {'Index':'index.html', 'About':'about.html'} %}
{% set navigation_list = ['index.html', 'about.html'] %}
{% set my_variable = foobar() %}
~~~

### Scoping Behavior

Please keep in mind that the scoping behavior of qrqma is the same as in python.
When a scope is exited the local variables will be promoted to the parent scope.
qrqma will even recognize if a constant was promoted and will treat the variable after the scope as a constant trying to pre-render as much of the template as possible.

This behavior is different to Jinja's scoping behavior!

## Extends

The extends tag can be used to extend one template from another. You can have multiple extends tags in a file, but only one of them may be executed at a time.

See the section about [Template Inheritance](#Template-Inheritance) above.

## Blocks

Blocks are used for inheritance and act as both placeholders and replacements at the same time. They are documented in detail in the [Template Inheritance](#Template-Inheritance) section.

# Expressions

qrqma allows basic expressions everywhere.
These work very similarly to regular Python; even if you’re not working with Python you should feel comfortable with it.

## Literals

The simplest form of expressions are literals.
Literals are representations for objects such as strings and numbers.
The following literals exist:

- ``"Hello World"``:  
  Everything between two double or single quotes is a string.
  They are useful whenever you need a string in the template (e.g. as arguments to function calls and filters, or just to extend or include a template).
- ``42 / 42.23``:  
  Integers and floating point numbers are created by just writing the number down.
  If a dot or exponent is present, the number is a float, otherwise an integer.
  Keep in mind that, in qrqma, 42 and 42.0 are different (int and float, respectively).
- ``[‘list’, ‘of’, ‘objects’]``:  
  Everything between two brackets is a list.
  Lists are useful for storing sequential data to be iterated over.
  For example, you can easily create a list of links using lists for (and with) a for loop:
  ~~~html
  <ul>
  {% for item in [['index.html', 'Index'], ['about.html', 'About'], ['downloads.html', 'Downloads']] %}
    <li><a href="{{ item[0] }}">{{ item[1] }}</a></li>
  {% endfor %}
  </ul>
  ~~~
- ``{‘dict’: ‘of’, ‘key’: ‘and’, ‘value’: ‘pairs’}``:  
  A dict in qrqma is a structure that combines keys and values.
  Keys must be unique strings and always have exactly one value.
- ``true`` / ``True`` / ``false`` / ``False``:  
  true is always True and false is always False.
  And vice versa.

## Math

qrqma allows you to calculate with values.
This is rarely useful in templates but exists for completeness’ sake.
The following operators are supported:

- ``+``  
  Adds two objects together.
  Usually the objects are numbers, but if both are strings or lists, you can concatenate them this way.
  ``{{ 1 + 1 }}`` is 2.
- ``-``  
  Subtract the second number from the first one. ``{{ 3 - 2 }}`` is 1.
- ``/``  
  Divide two numbers.
  The return value will have the same type as a division in C++. 
  ``{{ 1 / 2 }}`` is ``{{ 0 }}``, ``{{ 1. / 2 }}`` is ``{{ 0.5 }}``.
- ``%``  
  Calculate the remainder of an integer division.
  ``{{ 11 % 7 }}`` is 4.
- ``*``  
  Multiply the left operand with the right one.
  ``{{ 2 * 2 }}`` would return 4.
  
  
## Comparisons
- ``==``  
Compares two objects for equality.
- ``!=``  
Compares two objects for inequality.
- ``>``  
true if the left hand side is greater than the right hand side.
- ``>=``  
true if the left hand side is greater or equal to the right hand side.
- ``<``  
true if the left hand side is lower than the right hand side.
- ``<=``  
true if the left hand side is lower or equal to the right hand side.

## Logic

For if statements it can be useful to combine multiple expressions:
- ``and``/``&&``  
Return true if the left and the right operand are true.
- ``or``/``||``  
Return true if the left or the right operand are true.
- ``not``/``!``  
negate a statement.
- ``(expr)``  
group an expression.


There are quite a lot of features missing in qrqma that Jinja has implemented.
E.g., qrqma does not yet have builtins.
Those features will come in the future, though!

However, qrqma is already a quite versatile and usable template engine!
