FILE(GLOB_RECURSE app_sources *.c*)
target_sources(app PRIVATE ${app_sources})