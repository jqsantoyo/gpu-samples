#pragma once
#include <renderer/renderer.h>
#include <utils/utils.h>
#include <memory>
#include <vector>

namespace gpu {


std::unique_ptr<IRenderer> createRendererPbr();



}