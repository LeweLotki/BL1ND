#include "bluetooth_link.hpp"

#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_att.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "os/os_mbuf.h"
#include "services/gatt/ble_svc_gatt.h"

#include "esp_random.h"
#include "freertos/task.h"

#include <cstring>

namespace {

const ble_uuid128_t SERVICE_UUID = BLE_UUID128_INIT(
    0x21, 0x7b, 0x31, 0x8c, 0x5e, 0x93, 0x4f, 0xab,
    0x82, 0x61, 0x98, 0x40, 0x11, 0x2a, 0xd7, 0x01
);
const ble_uuid128_t CHARACTERISTIC_UUID = BLE_UUID128_INIT(
    0x21, 0x7b, 0x31, 0x8c, 0x5e, 0x93, 0x4f, 0xab,
    0x82, 0x61, 0x98, 0x40, 0x11, 0x2a, 0xd7, 0x02
);

uint16_t server_value_handle;
ble_gatt_chr_def characteristics[2];
ble_gatt_svc_def services[2];

} // namespace

BluetoothLink* BluetoothLink::instance_ = nullptr;

BluetoothLink::BluetoothLink(ChessGame& game)
    : event_queue_(xQueueCreate(QUEUE_LENGTH, sizeof(BluetoothEvent)))
    , session_(game, random, this)
    , started_(false)
    , host_ready_(false)
    , pairing_pending_(false)
    , hold_pending_(false)
    , connecting_(false)
    , subscribed_(false)
    , transport_ready_(false)
    , has_peer_address_(false)
    , connection_handle_(INVALID_HANDLE)
    , service_start_handle_(0)
    , service_end_handle_(0)
    , peer_value_handle_(0)
    , peer_cccd_handle_(0)
    , own_address_type_(0)
    , peer_address_type_(0)
    , peer_address_{}
    , service_data_{}
    , user_outputs_{}
    , user_output_head_(0)
    , user_output_count_(0)
{
}

bool BluetoothLink::start()
{
    if (started_) {
        return true;
    }
    if (event_queue_ == nullptr || instance_ != nullptr) {
        return false;
    }
    instance_ = this;
    if (nimble_port_init() != ESP_OK) {
        instance_ = nullptr;
        return false;
    }
    if (ble_att_set_preferred_mtu(128) != 0) {
        nimble_port_deinit();
        instance_ = nullptr;
        return false;
    }

    memset(characteristics, 0, sizeof(characteristics));
    characteristics[0].uuid = &CHARACTERISTIC_UUID.u;
    characteristics[0].access_cb = gattAccess;
    characteristics[0].flags =
        BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY;
    characteristics[0].val_handle = &server_value_handle;

    memset(services, 0, sizeof(services));
    services[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
    services[0].uuid = &SERVICE_UUID.u;
    services[0].characteristics = characteristics;

    ble_svc_gatt_init();
    if (ble_gatts_count_cfg(services) != 0
        || ble_gatts_add_svcs(services) != 0) {
        nimble_port_deinit();
        instance_ = nullptr;
        return false;
    }

    ble_hs_cfg.sync_cb = hostSync;
    ble_hs_cfg.reset_cb = hostReset;
    nimble_port_freertos_init(hostTask);
    started_ = true;
    return true;
}

QueueHandle_t BluetoothLink::queue() const
{
    return event_queue_;
}

bool BluetoothLink::receive(BluetoothEvent& event, TickType_t timeout)
{
    return xQueueReceive(event_queue_, &event, timeout) == pdTRUE;
}

uint32_t BluetoothLink::random(void*)
{
    return esp_random();
}

bool BluetoothLink::post(const BluetoothEvent& event)
{
    return xQueueSend(event_queue_, &event, 0) == pdTRUE;
}

void BluetoothLink::process(
    const BluetoothEvent& event,
    uint32_t now_ms
)
{
    switch (event.type) {
    case BluetoothEventType::HostReady:
        host_ready_ = true;
        if (hold_pending_) {
            hold_pending_ = false;
            session_.onEvent(LinkSession::holdEvent(), now_ms);
        }
        if (pairing_pending_) {
            startPairing();
        }
        break;
    case BluetoothEventType::HostReset:
        session_.onEvent(LinkSession::disconnectedEvent(), now_ms);
        break;
    case BluetoothEventType::PeerDiscovered:
        peer_address_type_ = event.address_type;
        memcpy(peer_address_, event.address, sizeof(peer_address_));
        has_peer_address_ = true;
        session_.onEvent(
            LinkSession::discoveredEvent(event.token),
            now_ms
        );
        break;
    case BluetoothEventType::Connected:
        if (event.status != 0) {
            session_.onEvent(LinkSession::disconnectedEvent(), now_ms);
            break;
        }
        ble_gap_conn_desc description;
        if (ble_gap_conn_find(event.first_handle, &description) != 0) {
            ble_gap_terminate(
                event.first_handle,
                BLE_ERR_REM_USER_CONN_TERM
            );
            break;
        }
        if (!connecting_ && has_peer_address_
            && (description.peer_ota_addr.type != peer_address_type_
                || memcmp(
                    description.peer_ota_addr.val,
                    peer_address_,
                    sizeof(peer_address_)
                ) != 0)) {
            ble_gap_terminate(
                event.first_handle,
                BLE_ERR_REM_USER_CONN_TERM
            );
            break;
        }
        peer_address_type_ = description.peer_ota_addr.type;
        memcpy(
            peer_address_,
            description.peer_ota_addr.val,
            sizeof(peer_address_)
        );
        has_peer_address_ = true;
        connection_handle_ = event.first_handle;
        subscribed_ = false;
        transport_ready_ = false;
        {
            ble_gap_upd_params parameters = {};
            parameters.itvl_min = 24;
            parameters.itvl_max = 40;
            parameters.latency = 0;
            parameters.supervision_timeout = 400;
            parameters.min_ce_len = 0;
            parameters.max_ce_len = 0;
            ble_gap_update_params(connection_handle_, &parameters);
        }
        if (connecting_) {
            discoverService();
        }
        break;
    case BluetoothEventType::Disconnected:
        connection_handle_ = INVALID_HANDLE;
        peer_value_handle_ = 0;
        peer_cccd_handle_ = 0;
        subscribed_ = false;
        transport_ready_ = false;
        session_.onEvent(LinkSession::disconnectedEvent(), now_ms);
        break;
    case BluetoothEventType::Service:
        service_start_handle_ = event.first_handle;
        service_end_handle_ = event.second_handle;
        break;
    case BluetoothEventType::ServiceDone:
        if (event.status == BLE_HS_EDONE && service_start_handle_ != 0) {
            discoverCharacteristic();
        }
        else if (event.status != 0 && event.status != BLE_HS_EDONE) {
            ble_gap_terminate(
                connection_handle_,
                BLE_ERR_REM_USER_CONN_TERM
            );
        }
        break;
    case BluetoothEventType::Characteristic:
        peer_value_handle_ = event.first_handle;
        break;
    case BluetoothEventType::CharacteristicDone:
        if (event.status == BLE_HS_EDONE && peer_value_handle_ != 0) {
            discoverDescriptor();
        }
        else if (event.status != 0 && event.status != BLE_HS_EDONE) {
            ble_gap_terminate(
                connection_handle_,
                BLE_ERR_REM_USER_CONN_TERM
            );
        }
        break;
    case BluetoothEventType::Descriptor:
        peer_cccd_handle_ = event.first_handle;
        break;
    case BluetoothEventType::DescriptorDone:
        if (event.status == BLE_HS_EDONE && peer_cccd_handle_ != 0) {
            enableNotifications();
        }
        else if (event.status != 0 && event.status != BLE_HS_EDONE) {
            ble_gap_terminate(
                connection_handle_,
                BLE_ERR_REM_USER_CONN_TERM
            );
        }
        break;
    case BluetoothEventType::SubscribeComplete:
        if (event.status == 0) {
            subscribed_ = true;
            exchangeMtu();
        }
        else {
            ble_gap_terminate(
                connection_handle_,
                BLE_ERR_REM_USER_CONN_TERM
            );
        }
        break;
    case BluetoothEventType::Mtu:
        if (event.status == 0 && event.mtu >= 64) {
            markTransportReady();
        }
        else {
            LinkOutput output = {};
            output.type = LinkOutputType::TransportError;
            output.token = event.mtu;
            saveUserOutput(output);
            ble_gap_terminate(
                connection_handle_,
                BLE_ERR_REM_USER_CONN_TERM
            );
        }
        break;
    case BluetoothEventType::Message:
        session_.onEvent(
            LinkSession::messageEvent(event.data, event.size),
            now_ms
        );
        break;
    }
    serviceOutputs();
}

void BluetoothLink::handleHold(uint32_t now_ms)
{
    if (!host_ready_) {
        hold_pending_ = true;
        return;
    }
    session_.onEvent(LinkSession::holdEvent(), now_ms);
    serviceOutputs();
}

void BluetoothLink::handleLocalMove(const Move& move, uint32_t now_ms)
{
    session_.onEvent(LinkSession::localMoveEvent(move), now_ms);
    serviceOutputs();
}

void BluetoothLink::handleLocalReset(uint32_t now_ms)
{
    session_.onEvent(LinkSession::localResetEvent(), now_ms);
    serviceOutputs();
}

void BluetoothLink::tick(uint32_t now_ms)
{
    session_.onEvent(LinkSession::tickEvent(), now_ms);
    serviceOutputs();
}

bool BluetoothLink::nextOutput(LinkOutput& output)
{
    serviceOutputs();
    if (user_output_count_ == 0) {
        return false;
    }
    output = user_outputs_[user_output_head_];
    user_output_head_ =
        (user_output_head_ + 1) % USER_OUTPUT_CAPACITY;
    --user_output_count_;
    return true;
}

LinkState BluetoothLink::state() const
{
    return session_.state();
}

void BluetoothLink::saveUserOutput(const LinkOutput& output)
{
    if (user_output_count_ == USER_OUTPUT_CAPACITY) {
        return;
    }
    const size_t index =
        (user_output_head_ + user_output_count_) % USER_OUTPUT_CAPACITY;
    user_outputs_[index] = output;
    ++user_output_count_;
}

void BluetoothLink::serviceOutputs()
{
    LinkOutput output = {};
    while (session_.nextOutput(output)) {
        switch (output.type) {
        case LinkOutputType::StartPairing:
            has_peer_address_ = false;
            startPairing();
            break;
        case LinkOutputType::RefreshPairing:
            startPairing();
            break;
        case LinkOutputType::ConnectToPeer:
            connectToPeer();
            break;
        case LinkOutputType::WaitForPeer:
            ble_gap_disc_cancel();
            break;
        case LinkOutputType::StopPairing:
            stopPairing();
            break;
        case LinkOutputType::Disconnect:
            if (connection_handle_ != INVALID_HANDLE) {
                ble_gap_terminate(
                    connection_handle_,
                    BLE_ERR_REM_USER_CONN_TERM
                );
            }
            break;
        case LinkOutputType::StartReconnect:
            startReconnect();
            break;
        case LinkOutputType::StopReconnect:
            stopPairing();
            break;
        case LinkOutputType::SendMessage:
            send(output.data, output.size);
            break;
        default:
            saveUserOutput(output);
            break;
        }
    }
}

void BluetoothLink::startPairing()
{
    pairing_pending_ = true;
    if (!host_ready_) {
        return;
    }
    stopPairing();
    memcpy(service_data_, SERVICE_UUID.value, sizeof(SERVICE_UUID.value));
    const uint32_t token = session_.token();
    service_data_[16] = static_cast<uint8_t>(token);
    service_data_[17] = static_cast<uint8_t>(token >> 8);
    service_data_[18] = static_cast<uint8_t>(token >> 16);
    service_data_[19] = static_cast<uint8_t>(token >> 24);

    ble_hs_adv_fields fields = {};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.svc_data_uuid128 = service_data_;
    fields.svc_data_uuid128_len = sizeof(service_data_);
    if (ble_gap_adv_set_fields(&fields) != 0) {
        return;
    }
    ble_gap_adv_params advertising = {};
    advertising.conn_mode = BLE_GAP_CONN_MODE_UND;
    advertising.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(
        own_address_type_,
        nullptr,
        BLE_HS_FOREVER,
        &advertising,
        gapEvent,
        this
    );

    ble_gap_disc_params scanning = {};
    scanning.filter_duplicates = 1;
    scanning.passive = 1;
    ble_gap_disc(
        own_address_type_,
        30000,
        &scanning,
        gapEvent,
        this
    );
}

void BluetoothLink::stopPairing()
{
    pairing_pending_ = false;
    ble_gap_disc_cancel();
    ble_gap_adv_stop();
}

void BluetoothLink::connectToPeer()
{
    stopPairing();
    connecting_ = true;
    ble_addr_t address = {};
    address.type = peer_address_type_;
    memcpy(address.val, peer_address_, sizeof(address.val));
    ble_gap_connect(
        own_address_type_,
        &address,
        30000,
        nullptr,
        gapEvent,
        this
    );
}

void BluetoothLink::startReconnect()
{
    if (!host_ready_) {
        return;
    }
    if (session_.isInitiator()) {
        connectToPeer();
    }
    else {
        connecting_ = false;
        pairing_pending_ = true;
        startPairing();
    }
}

void BluetoothLink::send(const uint8_t* data, size_t size)
{
    if (connection_handle_ == INVALID_HANDLE || size == 0) {
        return;
    }
    if (connecting_) {
        if (peer_value_handle_ != 0) {
            ble_gattc_write_flat(
                connection_handle_,
                peer_value_handle_,
                data,
                size,
                nullptr,
                nullptr
            );
        }
        return;
    }
    if (subscribed_) {
        os_mbuf* packet = ble_hs_mbuf_from_flat(data, size);
        if (packet != nullptr) {
            ble_gatts_notify_custom(
                connection_handle_,
                server_value_handle,
                packet
            );
        }
    }
}

void BluetoothLink::discoverService()
{
    service_start_handle_ = 0;
    service_end_handle_ = 0;
    ble_gattc_disc_svc_by_uuid(
        connection_handle_,
        &SERVICE_UUID.u,
        serviceDiscovered,
        this
    );
}

void BluetoothLink::discoverCharacteristic()
{
    peer_value_handle_ = 0;
    ble_gattc_disc_chrs_by_uuid(
        connection_handle_,
        service_start_handle_,
        service_end_handle_,
        &CHARACTERISTIC_UUID.u,
        characteristicDiscovered,
        this
    );
}

void BluetoothLink::discoverDescriptor()
{
    peer_cccd_handle_ = 0;
    ble_gattc_disc_all_dscs(
        connection_handle_,
        peer_value_handle_,
        service_end_handle_,
        descriptorDiscovered,
        this
    );
}

void BluetoothLink::enableNotifications()
{
    const uint8_t value[2] = { 1, 0 };
    ble_gattc_write_flat(
        connection_handle_,
        peer_cccd_handle_,
        value,
        sizeof(value),
        subscribed,
        this
    );
}

void BluetoothLink::exchangeMtu()
{
    if (ble_gattc_exchange_mtu(
            connection_handle_,
            mtuExchanged,
            this
        ) != 0) {
        ble_gap_terminate(
            connection_handle_,
            BLE_ERR_REM_USER_CONN_TERM
        );
    }
}

void BluetoothLink::markTransportReady()
{
    if (transport_ready_) {
        return;
    }
    transport_ready_ = true;
    session_.onEvent(
        LinkSession::connectedEvent(connecting_),
        xTaskGetTickCount() * portTICK_PERIOD_MS
    );
}

int BluetoothLink::gapEvent(ble_gap_event* event, void* argument)
{
    BluetoothLink* self = static_cast<BluetoothLink*>(argument);
    if (self == nullptr) {
        self = instance_;
    }
    if (self == nullptr) {
        return 0;
    }
    BluetoothEvent queued = {};
    switch (event->type) {
    case BLE_GAP_EVENT_DISC: {
        ble_hs_adv_fields fields = {};
        if (ble_hs_adv_parse_fields(
                &fields,
                event->disc.data,
                event->disc.length_data
            ) != 0
            || fields.svc_data_uuid128_len != sizeof(self->service_data_)
            || memcmp(
                fields.svc_data_uuid128,
                SERVICE_UUID.value,
                sizeof(SERVICE_UUID.value)
            ) != 0) {
            return 0;
        }
        queued.type = BluetoothEventType::PeerDiscovered;
        queued.address_type = event->disc.addr.type;
        memcpy(
            queued.address,
            event->disc.addr.val,
            sizeof(queued.address)
        );
        const uint8_t* token = fields.svc_data_uuid128 + 16;
        queued.token = static_cast<uint32_t>(token[0])
            | static_cast<uint32_t>(token[1]) << 8
            | static_cast<uint32_t>(token[2]) << 16
            | static_cast<uint32_t>(token[3]) << 24;
        self->post(queued);
        break;
    }
    case BLE_GAP_EVENT_CONNECT:
        queued.type = BluetoothEventType::Connected;
        queued.status = event->connect.status;
        queued.first_handle = event->connect.conn_handle;
        self->post(queued);
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        queued.type = BluetoothEventType::Disconnected;
        queued.status = event->disconnect.reason;
        self->post(queued);
        break;
    case BLE_GAP_EVENT_NOTIFY_RX:
        queued.type = BluetoothEventType::Message;
        queued.size = OS_MBUF_PKTLEN(event->notify_rx.om);
        if (queued.size <= sizeof(queued.data)
            && os_mbuf_copydata(
                event->notify_rx.om,
                0,
                queued.size,
                queued.data
            ) == 0) {
            self->post(queued);
        }
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (!self->connecting_ && event->subscribe.cur_notify) {
            queued.type = BluetoothEventType::SubscribeComplete;
            queued.status = 0;
            self->post(queued);
        }
        break;
    default:
        break;
    }
    return 0;
}

int BluetoothLink::gattAccess(
    uint16_t,
    uint16_t,
    ble_gatt_access_ctxt* context,
    void* argument
)
{
    BluetoothLink* self = static_cast<BluetoothLink*>(argument);
    if (self == nullptr) {
        self = instance_;
    }
    if (self == nullptr || context->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return 0;
    }
    BluetoothEvent event = {};
    event.type = BluetoothEventType::Message;
    event.size = OS_MBUF_PKTLEN(context->om);
    if (event.size > sizeof(event.data)
        || os_mbuf_copydata(
            context->om,
            0,
            event.size,
            event.data
        ) != 0
        || !self->post(event)) {
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    return 0;
}

int BluetoothLink::serviceDiscovered(
    uint16_t,
    const ble_gatt_error* error,
    const ble_gatt_svc* service,
    void* argument
)
{
    BluetoothLink* self = static_cast<BluetoothLink*>(argument);
    BluetoothEvent event = {};
    event.status = error->status;
    if (error->status == 0 && service != nullptr) {
        event.type = BluetoothEventType::Service;
        event.first_handle = service->start_handle;
        event.second_handle = service->end_handle;
    }
    else {
        event.type = BluetoothEventType::ServiceDone;
    }
    self->post(event);
    return 0;
}

int BluetoothLink::characteristicDiscovered(
    uint16_t,
    const ble_gatt_error* error,
    const ble_gatt_chr* characteristic,
    void* argument
)
{
    BluetoothLink* self = static_cast<BluetoothLink*>(argument);
    BluetoothEvent event = {};
    event.status = error->status;
    if (error->status == 0 && characteristic != nullptr) {
        event.type = BluetoothEventType::Characteristic;
        event.first_handle = characteristic->val_handle;
    }
    else {
        event.type = BluetoothEventType::CharacteristicDone;
    }
    self->post(event);
    return 0;
}

int BluetoothLink::descriptorDiscovered(
    uint16_t,
    const ble_gatt_error* error,
    uint16_t,
    const ble_gatt_dsc* descriptor,
    void* argument
)
{
    BluetoothLink* self = static_cast<BluetoothLink*>(argument);
    BluetoothEvent event = {};
    event.status = error->status;
    if (error->status == 0 && descriptor != nullptr) {
        if (ble_uuid_u16(&descriptor->uuid.u)
            != BLE_GATT_DSC_CLT_CFG_UUID16) {
            return 0;
        }
        event.type = BluetoothEventType::Descriptor;
        event.first_handle = descriptor->handle;
    }
    else {
        event.type = BluetoothEventType::DescriptorDone;
    }
    self->post(event);
    return 0;
}

int BluetoothLink::subscribed(
    uint16_t,
    const ble_gatt_error* error,
    ble_gatt_attr*,
    void* argument
)
{
    BluetoothLink* self = static_cast<BluetoothLink*>(argument);
    BluetoothEvent event = {};
    event.type = BluetoothEventType::SubscribeComplete;
    event.status = error->status;
    self->post(event);
    return 0;
}

int BluetoothLink::mtuExchanged(
    uint16_t,
    const ble_gatt_error* error,
    uint16_t mtu,
    void* argument
)
{
    BluetoothLink* self = static_cast<BluetoothLink*>(argument);
    BluetoothEvent event = {};
    event.type = BluetoothEventType::Mtu;
    event.status = error->status;
    event.mtu = mtu;
    self->post(event);
    return 0;
}

void BluetoothLink::hostTask(void*)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void BluetoothLink::hostSync()
{
    if (instance_ == nullptr) {
        return;
    }
    if (ble_hs_util_ensure_addr(0) != 0
        || ble_hs_id_infer_auto(
            0,
            &instance_->own_address_type_
        ) != 0) {
        hostReset(-1);
        return;
    }
    BluetoothEvent event = {};
    event.type = BluetoothEventType::HostReady;
    instance_->post(event);
}

void BluetoothLink::hostReset(int reason)
{
    if (instance_ == nullptr) {
        return;
    }
    BluetoothEvent event = {};
    event.type = BluetoothEventType::HostReset;
    event.status = reason;
    instance_->post(event);
}
