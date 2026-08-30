#pragma once

class StandardOutput;

class WifiAccessPoint {
public:
    explicit WifiAccessPoint(StandardOutput& output);

    bool start();

private:
    StandardOutput& output_;
};
