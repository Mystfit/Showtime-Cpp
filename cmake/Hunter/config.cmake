# Local hunter packages until changes are merged
# hunter_config(
#     czmq
#     VERSION 4.1.0
#     URL "https://github.com/Mystfit/czmq/archive/v4.1.0-hunter.tar.gz"
#     SHA1 "2ca943a2e3d83b53d338301c7f6e48f8adc78ca9"
# )

# hunter_config(
#     log4cplus
#     VERSION 2.0.x
#     URL "https://github.com/Mystfit/log4cplus/archive/REL_2_0_0-RC2-hunter.tar.gz"
#     SHA1 "ef4f31e58cbd4f88e25c87f035d77553270"
# )

# hunter_config(
#     msgpack-c
#     VERSION 2.1.5
#     URL "https://github.com/Mystfit/log4cplus/archive/REL_2_0_0-RC2.tar.gz"
#     SHA1 "66e309028d0aaf1c70b70900fb366d191bbffc1"
# )

hunter_config(Boost VERSION 1.66.0
	CMAKE_ARGS
	# BUILD_SHARED_LIBS=ON
	# Boost_USE_STATIC_LIBS=OFF
	# Boost_USE_STATIC_LIBS=OFF
	# BOOST_ALL_DYN_LINK=ON
	# BOOST_FILESYSTEM_DYN_LINK=OFF
	# BOOST_SYSTEM_DYN_LINK=OFF
	# BOOST_LOG_DYN_LINK=ON
)