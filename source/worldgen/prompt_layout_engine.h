#ifndef RME_WORLDGEN_PROMPT_LAYOUT_ENGINE_H_
#define RME_WORLDGEN_PROMPT_LAYOUT_ENGINE_H_

#include "worldgen/worldgen_types.h"

#include <string>

class PromptLayoutEngine {
public:
	PromptDraft buildDraft(const std::string& prompt) const;

private:
	static std::string toLower(std::string text);
};

#endif
