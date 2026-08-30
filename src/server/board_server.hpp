#pragma once

#include "esp_http_server.h"

class BoardSnapshot;
class StandardOutput;

class BoardServer {
public:
    BoardServer(BoardSnapshot& board_snapshot, StandardOutput& output);

    bool start();

private:
    static esp_err_t handleRoot(httpd_req_t* request);

    BoardSnapshot& board_snapshot_;
    StandardOutput& output_;
    httpd_handle_t server_;
};
