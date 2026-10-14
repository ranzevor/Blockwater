#pragma once
#include <stdexcept>
#define REQUIRE(condition) do { if (!(condition)) throw std::runtime_error(__FILE__ ":" + std::to_string(__LINE__) + ": " #condition); } while (false)
