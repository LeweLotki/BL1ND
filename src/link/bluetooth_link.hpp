#pragma once

#include "link_session.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <cstddef>
#include <cstdint>

struct ble_gap_event;
struct ble_gatt_access_ctxt;
struct ble_gatt_attr;
struct ble_gatt_chr;
struct ble_gatt_dsc;
struct ble_gatt_error;
struct ble_gatt_svc;

enum class BluetoothEventType : uint8_t {
    HostReady,
    HostReset,
    PeerDiscovered,
    Connected,
    Disconnected,
    Service,
    ServiceDone,
    Characteristic,
    CharacteristicDone,
    Descriptor,
    DescriptorDone,
    SubscribeComplete,
    Mtu,
    Message,
};

struct BluetoothEvent {
    BluetoothEventType type;
    int status;
    uint16_t first_handle;
    uint16_t second_handle;
    uint16_t mtu;
    uint32_t token;
    uint8_t address_type;
    uint8_t address[6];
    uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
    size_t size;
};

class BluetoothLink {
public:
    static constexpr UBaseType_t QUEUE_LENGTH = 12;

    explicit BluetoothLink(ChessGame& game);

    bool start();
    QueueHandle_t queue() const;
    bool receive(BluetoothEvent& event, TickType_t timeout);

    void process(const BluetoothEvent& event, uint32_t now_ms);
    void handleHold(uint32_t now_ms);
    void handleLocalMove(const Move& move, uint32_t now_ms);
    void handleLocalReset(uint32_t now_ms);
    void tick(uint32_t now_ms);
    bool nextOutput(LinkOutput& output);

    LinkState state() const;

    static int gapEvent(ble_gap_event* event, void* argument);
    static int gattAccess(
        uint16_t connection_handle,
        uint16_t attribute_handle,
        ble_gatt_access_ctxt* context,
        void* argument
    );
    static int serviceDiscovered(
        uint16_t connection_handle,
        const ble_gatt_error* error,
        const ble_gatt_svc* service,
        void* argument
    );
    static int characteristicDiscovered(
        uint16_t connection_handle,
        const ble_gatt_error* error,
        const ble_gatt_chr* characteristic,
        void* argument
    );
    static int descriptorDiscovered(
        uint16_t connection_handle,
        const ble_gatt_error* error,
        uint16_t characteristic_handle,
        const ble_gatt_dsc* descriptor,
        void* argument
    );
    static int subscribed(
        uint16_t connection_handle,
        const ble_gatt_error* error,
        ble_gatt_attr* attribute,
        void* argument
    );
    static int mtuExchanged(
        uint16_t connection_handle,
        const ble_gatt_error* error,
        uint16_t mtu,
        void* argument
    );
    static void hostTask(void* argument);
    static void hostSync();
    static void hostReset(int reason);

private:
    static constexpr size_t USER_OUTPUT_CAPACITY = 16;
    static constexpr uint16_t INVALID_HANDLE = 0xffff;

    static uint32_t random(void* context);
    bool post(const BluetoothEvent& event);
    void serviceOutputs();
    void saveUserOutput(const LinkOutput& output);
    void startPairing();
    void stopPairing();
    void connectToPeer();
    void startReconnect();
    void send(const uint8_t* data, size_t size);
    void discoverService();
    void discoverCharacteristic();
    void discoverDescriptor();
    void enableNotifications();
    void exchangeMtu();
    void markTransportReady();

    static BluetoothLink* instance_;

    QueueHandle_t event_queue_;
    LinkSession session_;
    bool started_;
    bool host_ready_;
    bool pairing_pending_;
    bool hold_pending_;
    bool connecting_;
    bool subscribed_;
    bool transport_ready_;
    bool has_peer_address_;
    uint16_t connection_handle_;
    uint16_t service_start_handle_;
    uint16_t service_end_handle_;
    uint16_t peer_value_handle_;
    uint16_t peer_cccd_handle_;
    uint8_t own_address_type_;
    uint8_t peer_address_type_;
    uint8_t peer_address_[6];
    uint8_t service_data_[20];
    LinkOutput user_outputs_[USER_OUTPUT_CAPACITY];
    size_t user_output_head_;
    size_t user_output_count_;
};
