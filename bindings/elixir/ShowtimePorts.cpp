#include <Showtime.h>

int main(int argc, char **argv)
{
	ZstLog::init_file_logging();
	zst_init("showtime_ports", false);
}
