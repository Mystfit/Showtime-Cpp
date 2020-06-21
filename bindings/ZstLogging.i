namespace showtime {
	%ignore ZstLog::LoggerInfo;
	%feature("director") ZstLog::ExtLog;
}

%include <showtime/ZstLogging.h>
