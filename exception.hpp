#pragma once

#include <exception>


struct SyntaxError: std::exception {
};

struct RuntimeError: std::exception {
};

struct IOError: std::exception {
};
