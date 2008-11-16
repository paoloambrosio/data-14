#ifndef DEBUG_H_
#define DEBUG_H_

#if DBG

#define LOG_LEVEL LOG_DEBUG

#define LOG_DEBUG   1
#define LOG_INFO    2
#define LOG_WARNING 3
#define LOG_ERROR   4
#define LOG_FATAL   5

#define LOG(level, msg, ...) { \
		if (LOG_LEVEL <= (level)) { \
			DbgPrint(msg "\n", __VA_ARGS__); \
		} \
	}

#define LOG_EX(level, prefix, msg, ...) { \
		if (LOG_LEVEL <= (level)) { \
			DbgPrint("%s %s (line %d): " msg "\n", prefix, __FUNCTION__, __LINE__, __VA_ARGS__); \
		} \
	}

#define DEBUG(msg, ...) LOG(LOG_DEBUG, msg, __VA_ARGS__)
#define INFO(msg, ...) LOG(LOG_INFO, msg, __VA_ARGS__)
#define WARNING(msg, ...) LOG_EX(LOG_WARNING, "WRN **", msg, __VA_ARGS__)
#define ERROR(msg, ...) LOG_EX(LOG_ERROR, "ERR **", msg, __VA_ARGS__)
#define FATAL(msg, ...) { \
		LOG_EX(LOG_FATAL, "RIP **", msg, __VA_ARGS__); \
		DbgBreakPoint(); \
	}

#define DEBUG_IF(cond, msg, ...) { if (cond) DEBUG(msg, __VA_ARGS__) }
#define INFO_IF(cond, msg, ...) { if (cond) INFO(msg, __VA_ARGS__) }
#define WARNING_IF(cond, msg, ...) { if (cond) WARNING(msg, __VA_ARGS__) }
#define ERROR_IF(cond, msg, ...) { if (cond) ERROR(msg, __VA_ARGS__) }
#define FATAL_IF(cond,msg, ...) { if (cond) FATAL(msg, __VA_ARGS__) }

#else

#define LOG(level, msg, ...)
#define LOG_EX(level, prefix, msg, ...)

#define DEBUG(msg, ...)
#define INFO(msg, ...)
#define WARNING(msg, ...)
#define ERROR(msg, ...)
#define FATAL(msg, ...)

#define DEBUG_IF(cond, msg, ...)
#define INFO_IF(cond, msg, ...)
#define WARNING_IF(cond, msg, ...)
#define ERROR_IF(cond, msg, ...)
#define FATAL_IF(cond, msg, ...)

#endif

#endif
