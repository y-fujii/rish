// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <exception>

using namespace std;


struct SyntaxError: exception {
};

struct RuntimeError: exception {
};

struct IOError: exception {
};
