#pragma once
#include <stdexcept>
#include <string>
#define REQUIRE(condition) do { if (!(condition)) throw std::runtime_error(__FILE__ ":" + std::to_string(__LINE__) + ": " #condition); } while (false)
