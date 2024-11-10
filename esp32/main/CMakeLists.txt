idf_component_register(
  SRCS
    esp32_capture.c
    main.c
    sdmmc.c
    wifi.c
    ../../src/avi_parse.c
    ../../src/avi_play.c
    ../../src/config.c
    ../../src/file_io.c
    ../../src/general.c
    ../../src/http_headers.c
    ../../src/mime_types.c
    ../../src/network_io.c
    ../../src/server.c
    ../../src/url_utils.c
    ../../src/user.c
  INCLUDE_DIRS
    ../../src
)

add_definitions(
  -DDEBUG
  -DENABLE_CAPTURE
  -DENABLE_ESP32
)

