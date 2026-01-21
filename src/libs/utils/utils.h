#pragma once
#include <memory>
#include <string>

namespace gpu {

std::wstring getAssetsPathW();
std::string getAssetsPath();
uint32_t align256(uint32_t x);

}