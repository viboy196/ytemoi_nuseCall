cd /D C:\Users\viboy\Downloads\Workspace\Workspace\test\voip\build || exit /b
C:\esp\.espressif\python_env\idf3.3_py3.8_env\Scripts\python.exe C:/esp/esp-idf/tools/kconfig_new/confgen.py --kconfig C:/esp/esp-idf/Kconfig --config C:/Users/viboy/Downloads/Workspace/Workspace/test/voip/sdkconfig --defaults C:/Users/viboy/Downloads/Workspace/Workspace/test/voip/sdkconfig.defaults --env "COMPONENT_KCONFIGS= C:/esp/esp-idf/components/app_trace/Kconfig C:/esp/esp-idf/components/aws_iot/Kconfig C:/esp/esp-idf/components/bt/Kconfig C:/esp/esp-idf/components/driver/Kconfig C:/esp/esp-idf/components/efuse/Kconfig C:/esp/esp-idf/components/esp32/Kconfig C:/esp/esp-idf/components/esp_adc_cal/Kconfig C:/esp/esp-idf/components/esp_event/Kconfig C:/esp/esp-idf/components/esp_http_client/Kconfig C:/esp/esp-idf/components/esp_http_server/Kconfig C:/esp/esp-idf/components/esp_https_ota/Kconfig C:/esp/esp-idf/components/espcoredump/Kconfig C:/esp/esp-idf/components/ethernet/Kconfig C:/esp/esp-idf/components/fatfs/Kconfig C:/esp/esp-idf/components/freemodbus/Kconfig C:/esp/esp-idf/components/freertos/Kconfig C:/esp/esp-idf/components/heap/Kconfig C:/esp/esp-idf/components/libsodium/Kconfig C:/esp/esp-idf/components/log/Kconfig C:/esp/esp-idf/components/lwip/Kconfig C:/esp/esp-idf/components/mbedtls/Kconfig C:/esp/esp-idf/components/mdns/Kconfig C:/esp/esp-idf/components/mqtt/Kconfig C:/esp/esp-idf/components/nvs_flash/Kconfig C:/esp/esp-idf/components/openssl/Kconfig C:/esp/esp-idf/components/pthread/Kconfig C:/esp/esp-idf/components/spi_flash/Kconfig C:/esp/esp-idf/components/spiffs/Kconfig C:/esp/esp-idf/components/tcpip_adapter/Kconfig C:/esp/esp-idf/components/unity/Kconfig C:/esp/esp-idf/components/vfs/Kconfig C:/esp/esp-idf/components/wear_levelling/Kconfig C:/esp/esp-idf/components/wifi_provisioning/Kconfig C:/esp/esp-idf/components/wpa_supplicant/Kconfig" --env "COMPONENT_KCONFIGS_PROJBUILD= C:/esp/esp-idf/components/app_update/Kconfig.projbuild C:/esp/.espressif/esp-adf/components/audio_board/Kconfig.projbuild C:/esp/esp-idf/components/bootloader/Kconfig.projbuild C:/esp/.espressif/esp-adf/components/esp-adf-libs/Kconfig.projbuild C:/esp/.espressif/esp-adf/components/esp-sr/Kconfig.projbuild C:/esp/.espressif/esp-adf/components/esp_dispatcher/Kconfig.projbuild C:/esp/esp-idf/components/esptool_py/Kconfig.projbuild C:/Users/viboy/Downloads/Workspace/Workspace/test/voip/main/Kconfig.projbuild C:/esp/esp-idf/components/partition_table/Kconfig.projbuild" --env IDF_CMAKE=y --env IDF_TARGET=esp32 --output config C:/Users/viboy/Downloads/Workspace/Workspace/test/voip/sdkconfig || exit /b
C:\esp\.espressif\tools\cmake\3.13.4\bin\cmake.exe -E env "COMPONENT_KCONFIGS= C:/esp/esp-idf/components/app_trace/Kconfig C:/esp/esp-idf/components/aws_iot/Kconfig C:/esp/esp-idf/components/bt/Kconfig C:/esp/esp-idf/components/driver/Kconfig C:/esp/esp-idf/components/efuse/Kconfig C:/esp/esp-idf/components/esp32/Kconfig C:/esp/esp-idf/components/esp_adc_cal/Kconfig C:/esp/esp-idf/components/esp_event/Kconfig C:/esp/esp-idf/components/esp_http_client/Kconfig C:/esp/esp-idf/components/esp_http_server/Kconfig C:/esp/esp-idf/components/esp_https_ota/Kconfig C:/esp/esp-idf/components/espcoredump/Kconfig C:/esp/esp-idf/components/ethernet/Kconfig C:/esp/esp-idf/components/fatfs/Kconfig C:/esp/esp-idf/components/freemodbus/Kconfig C:/esp/esp-idf/components/freertos/Kconfig C:/esp/esp-idf/components/heap/Kconfig C:/esp/esp-idf/components/libsodium/Kconfig C:/esp/esp-idf/components/log/Kconfig C:/esp/esp-idf/components/lwip/Kconfig C:/esp/esp-idf/components/mbedtls/Kconfig C:/esp/esp-idf/components/mdns/Kconfig C:/esp/esp-idf/components/mqtt/Kconfig C:/esp/esp-idf/components/nvs_flash/Kconfig C:/esp/esp-idf/components/openssl/Kconfig C:/esp/esp-idf/components/pthread/Kconfig C:/esp/esp-idf/components/spi_flash/Kconfig C:/esp/esp-idf/components/spiffs/Kconfig C:/esp/esp-idf/components/tcpip_adapter/Kconfig C:/esp/esp-idf/components/unity/Kconfig C:/esp/esp-idf/components/vfs/Kconfig C:/esp/esp-idf/components/wear_levelling/Kconfig C:/esp/esp-idf/components/wifi_provisioning/Kconfig C:/esp/esp-idf/components/wpa_supplicant/Kconfig" "COMPONENT_KCONFIGS_PROJBUILD= C:/esp/esp-idf/components/app_update/Kconfig.projbuild C:/esp/.espressif/esp-adf/components/audio_board/Kconfig.projbuild C:/esp/esp-idf/components/bootloader/Kconfig.projbuild C:/esp/.espressif/esp-adf/components/esp-adf-libs/Kconfig.projbuild C:/esp/.espressif/esp-adf/components/esp-sr/Kconfig.projbuild C:/esp/.espressif/esp-adf/components/esp_dispatcher/Kconfig.projbuild C:/esp/esp-idf/components/esptool_py/Kconfig.projbuild C:/Users/viboy/Downloads/Workspace/Workspace/test/voip/main/Kconfig.projbuild C:/esp/esp-idf/components/partition_table/Kconfig.projbuild" IDF_CMAKE=y KCONFIG_CONFIG=C:/Users/viboy/Downloads/Workspace/Workspace/test/voip/sdkconfig IDF_TARGET=esp32 C:/esp/.espressif/tools/mconf/v4.6.0.0-idf-20190628/mconf-idf.exe C:/esp/esp-idf/Kconfig || exit /b
