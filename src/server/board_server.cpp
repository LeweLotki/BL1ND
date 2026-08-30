#include "board_server.hpp"

#include "board_page.hpp"
#include "board_snapshot.hpp"
#include "standard_output.hpp"

#include "esp_err.h"

namespace {

// The ESP-IDF HTTP server runs URI handlers serially on one server task.
// This buffer must become per-request if handlers are ever made concurrent.
char PAGE_BUFFER[4096];

} // namespace

BoardServer::BoardServer(
    BoardSnapshot& board_snapshot,
    StandardOutput& output
)
    : board_snapshot_(board_snapshot)
    , output_(output)
    , server_(nullptr)
{
}

bool BoardServer::start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = 3;

    esp_err_t error = httpd_start(&server_, &config);
    if (error != ESP_OK) {
        output_.printf(
            "Preview: HTTP server failed: %s\n",
            esp_err_to_name(error)
        );
        server_ = nullptr;
        return false;
    }

    httpd_uri_t root = {};
    root.uri = "/";
    root.method = HTTP_GET;
    root.handler = handleRoot;
    root.user_ctx = this;
    error = httpd_register_uri_handler(server_, &root);
    if (error != ESP_OK) {
        output_.printf(
            "Preview: registering / failed: %s\n",
            esp_err_to_name(error)
        );
        const esp_err_t stop_error = httpd_stop(server_);
        if (stop_error != ESP_OK) {
            output_.printf(
                "Preview: HTTP stop failed: %s\n",
                esp_err_to_name(stop_error)
            );
        }
        server_ = nullptr;
        return false;
    }

    output_.print("Preview: HTTP server started\n");
    return true;
}

esp_err_t BoardServer::handleRoot(httpd_req_t* request)
{
    BoardServer* server = static_cast<BoardServer*>(request->user_ctx);

    char cells[64];
    if (!server->board_snapshot_.read(cells)) {
        return httpd_resp_send_err(
            request,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Board is not ready"
        );
    }

    const size_t page_size = renderBoardPage(
        cells,
        PAGE_BUFFER,
        sizeof(PAGE_BUFFER)
    );
    if (page_size == 0) {
        return httpd_resp_send_err(
            request,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Board page is too large"
        );
    }

    esp_err_t error = httpd_resp_set_type(request, "text/html");
    if (error != ESP_OK) {
        return error;
    }
    error = httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    if (error != ESP_OK) {
        return error;
    }

    return httpd_resp_send(
        request,
        PAGE_BUFFER,
        static_cast<ssize_t>(page_size)
    );
}
