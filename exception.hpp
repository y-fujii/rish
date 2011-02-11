#pragma once

#include <exception>


struct IOError: std::exception {
};

struct SyntaxError: std::exception {
};
