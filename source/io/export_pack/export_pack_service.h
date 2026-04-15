#ifndef RME_EXPORT_PACK_SERVICE_H_
#define RME_EXPORT_PACK_SERVICE_H_

#include <string>

class Map;

class ExportPackService {
public:
	static bool writeMetadataPackage(const Map& map, const std::string& output_directory, std::string& error);
};

#endif
