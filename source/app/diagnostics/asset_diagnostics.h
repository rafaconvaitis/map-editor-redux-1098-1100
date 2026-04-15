#ifndef RME_ASSET_DIAGNOSTICS_H_
#define RME_ASSET_DIAGNOSTICS_H_

#include <string>
#include <vector>

class ClientVersion;

struct AssetDiagnosticsReport {
	bool ok = true;
	std::vector<std::string> issues;
	std::vector<std::string> repair_hints;
};

class AssetDiagnostics {
public:
	static AssetDiagnosticsReport analyze(const ClientVersion& version);
};

#endif
