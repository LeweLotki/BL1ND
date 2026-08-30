#include "wifi_access_point.hpp"

#include "standard_output.hpp"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_defaults.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs_flash.h"

#include <cstring>

namespace {

constexpr char SSID[] = "esp_chessboard";

bool reportError(
    StandardOutput& output,
    const char* operation,
    esp_err_t error
)
{
    if (error == ESP_OK) {
        return false;
    }

    output.printf(
        "Preview: %s failed: %s\n",
        operation,
        esp_err_to_name(error)
    );
    return true;
}

} // namespace

WifiAccessPoint::WifiAccessPoint(StandardOutput& output)
    : output_(output)
{
}

bool WifiAccessPoint::start()
{
    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES
        || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        error = nvs_flash_erase();
        if (reportError(output_, "nvs_flash_erase", error)) {
            return false;
        }
        error = nvs_flash_init();
    }
    if (reportError(output_, "nvs_flash_init", error)) {
        return false;
    }

    error = esp_netif_init();
    if (reportError(output_, "esp_netif_init", error)) {
        return false;
    }

    error = esp_event_loop_create_default();
    if (reportError(output_, "event loop", error)) {
        return false;
    }

    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_AP();
    esp_netif_t* netif = esp_netif_new(&netif_config);
    if (netif == nullptr) {
        output_.print("Preview: failed to create Wi-Fi AP interface\n");
        return false;
    }

    error = esp_netif_attach_wifi_ap(netif);
    if (reportError(output_, "attach Wi-Fi AP interface", error)) {
        esp_netif_destroy_default_wifi(netif);
        return false;
    }

    error = esp_wifi_set_default_wifi_ap_handlers();
    if (reportError(output_, "register Wi-Fi AP handlers", error)) {
        esp_netif_destroy_default_wifi(netif);
        return false;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    error = esp_wifi_init(&init_config);
    if (reportError(output_, "esp_wifi_init", error)) {
        return false;
    }

    error = esp_wifi_set_mode(WIFI_MODE_AP);
    if (reportError(output_, "esp_wifi_set_mode", error)) {
        return false;
    }

    wifi_config_t wifi_config = {};
    memcpy(wifi_config.ap.ssid, SSID, sizeof(SSID) - 1);
    wifi_config.ap.ssid_len = sizeof(SSID) - 1;
    wifi_config.ap.channel = 1;
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    wifi_config.ap.ssid_hidden = 0;
    wifi_config.ap.max_connection = 4;

    error = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (reportError(output_, "esp_wifi_set_config", error)) {
        return false;
    }

    error = esp_wifi_start();
    if (reportError(output_, "esp_wifi_start", error)) {
        return false;
    }

    output_.printf(
        "Preview: join %s and open http://192.168.4.1/\n",
        SSID
    );
    return true;
}
