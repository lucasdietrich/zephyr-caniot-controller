# add_subdirectory(canif)
# add_subdirectory(cantcp)
# add_subdirectory(cantest)

add_subdirectory(discovery_server)
add_subdirectory(drivers)
add_subdirectory(ha)
add_subdirectory(http_server)
add_subdirectory(userio)
add_subdirectory(utils)
add_subdirectory(settings)

FILE(GLOB app_sources *.c*)
target_sources(app PRIVATE ${app_sources})