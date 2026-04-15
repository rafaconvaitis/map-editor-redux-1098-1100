#ifndef RME_PRESAVE_VALIDATOR_H_
#define RME_PRESAVE_VALIDATOR_H_

#include <string>
#include <vector>

class Map;

enum class PreSaveIssueSeverity {
	Info,
	Warning,
	Error
};

struct PreSaveIssue {
	PreSaveIssueSeverity severity = PreSaveIssueSeverity::Info;
	std::string code;
	std::string message;
	int count = 0;
};

struct PreSaveValidationReport {
	std::vector<PreSaveIssue> issues;
	int error_count = 0;
	int warning_count = 0;

	bool hasBlockingErrors() const {
		return error_count > 0;
	}
	std::vector<std::string> toLines() const;
};

class PreSaveValidator {
public:
	static PreSaveValidationReport validate(Map& map);
	static PreSaveValidationReport autoFix(Map& map);
};

#endif
