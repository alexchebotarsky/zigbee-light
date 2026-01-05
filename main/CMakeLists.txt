file(GLOB_RECURSE SOURCES "*.cpp")
idf_component_register(
  SRCS ${SOURCES}
  INCLUDE_DIRS "."
  PRIV_REQUIRES nvs_flash
)
