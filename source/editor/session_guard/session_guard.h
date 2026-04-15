#ifndef RME_SESSION_GUARD_H_
#define RME_SESSION_GUARD_H_

#include <string>

class Map;

class SessionGuard {
public:
	static bool acquire(const std::string& map_file_path, std::string& error);
	static void release(const std::string& map_file_path);
	static void writeTransactionalAutosave(Map& map, const std::string& map_file_path);
};

#endif
