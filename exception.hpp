// (c) Yasuhiro Fujii <y-fujii at mimosa-pudica.net> / 2-clause BSD license
#pragma once

#include <exception>


struct SyntaxError: std::exception {
};

struct RuntimeError: std::exception {
};

struct IOError: std::exception {
};
