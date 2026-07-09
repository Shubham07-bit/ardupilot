#include "AP_Mount_config.h"

#if HAL_MOUNT_K40T_ENABLED

#include "AP_Mount_K40T.h"

#include <AP_HAL/AP_HAL.h>
#include <AP_AHRS/AP_AHRS.h>
#include <AP_GPS/AP_GPS.h>
#include <GCS_MAVLink/GCS.h>

extern const AP_HAL::HAL& hal;

#define AP_MOUNT_K40T_STX           0xFD
#define AP_MOUNT_K40T_SYSID_FC      0x02
#define AP_MOUNT_K40T_COMPID_FC     0x01
#define AP_MOUNT_K40T_SYSID_CAM     0x04
#define AP_MOUNT_K40T_SYSID_GIMBAL  0x03
#define AP_MOUNT_K40T_TIMEOUT_MS    1000    // timeout for health and rangefinder readings
#define AP_MOUNT_K40T_THERM_TIMEOUT_MS 3000 // timeout for thermal readings

#define AP_MOUNT_K40T_DEBUG 0
#define debug(fmt, args ...) do { if (AP_MOUNT_K40T_DEBUG) { GCS_SEND_TEXT(MAV_SEVERITY_INFO, "K40T: " fmt, ## args); } } while (0)

// update mount position - should be called periodically
void AP_Mount_K40T::update()
{
    AP_Mount_Backend::update();

    // exit immediately if not initialised
    if (!_initialised) {
        return;
    }

    // reading incoming packets from gimbal/camera
    read_incoming_packets();

    // update zoom control
    update_zoom_control();

    // update based on mount mode
    update_mnt_target();

    // send target angles depending on the target type
    send_target_to_gimbal();
}

// return true if healthy
bool AP_Mount_K40T::healthy() const
{
    // unhealthy until initialised
    if (!_initialised) {
        return false;
    }

    // unhealthy if attitude information not received recently
    const uint32_t now_ms = AP_HAL::millis();
    if (now_ms - _last_attitude_ms > AP_MOUNT_K40T_TIMEOUT_MS) {
        return false;
    }

    return true;
}

// get attitude as a quaternion.  returns true on success
bool AP_Mount_K40T::get_attitude_quaternion(Quaternion& att_quat)
{
    att_quat.from_euler(_current_angle_rad.x, _current_angle_rad.y, _current_angle_rad.z);
    return true;
}

// reading incoming packets from gimbal/camera and confirm they are of the correct format
void AP_Mount_K40T::read_incoming_packets()
{
    // check for bytes on the serial port
    int16_t nbytes = MIN(_uart->available(), 1024U);
    if (nbytes <= 0) {
        return;
    }

    bool reset_parser = false;

    for (int16_t i = 0; i < nbytes; i++) {
        uint8_t b;
        if (!_uart->read(b)) {
            continue;
        }

        // process byte depending upon current state
        switch (_parsed_msg.state) {

        case ParseState::WAITING_FOR_STX:
            if (b == AP_MOUNT_K40T_STX) {
                _parsed_msg.state = ParseState::WAITING_FOR_LEN;
                _msg_buff_len = 0;
                _msg_buff[_msg_buff_len++] = b;
            }
            break;

        case ParseState::WAITING_FOR_LEN:
            _parsed_msg.payload_len = b;
            _msg_buff[_msg_buff_len++] = b;
            if (_parsed_msg.payload_len <= AP_MOUNT_K40T_PAYLOAD_MAX) {
                _parsed_msg.state = ParseState::WAITING_FOR_SYSID_RX;
            } else {
                reset_parser = true;
            }
            break;

        case ParseState::WAITING_FOR_SYSID_RX:
            _parsed_msg.sysid_rx = b;
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.state = ParseState::WAITING_FOR_COMPID_RX;
            break;

        case ParseState::WAITING_FOR_COMPID_RX:
            _parsed_msg.compid_rx = b;
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.state = ParseState::WAITING_FOR_SEQ;
            break;

        case ParseState::WAITING_FOR_SEQ:
            _parsed_msg.seq = b;
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.state = ParseState::WAITING_FOR_SYSID_TX;
            break;

        case ParseState::WAITING_FOR_SYSID_TX:
            _parsed_msg.sysid_tx = b;
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.state = ParseState::WAITING_FOR_COMPID_TX;
            break;

        case ParseState::WAITING_FOR_COMPID_TX:
            _parsed_msg.compid_tx = b;
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.state = ParseState::WAITING_FOR_MSGID0;
            break;

        case ParseState::WAITING_FOR_MSGID0:
            _parsed_msg.msgid[0] = b;
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.state = ParseState::WAITING_FOR_MSGID1;
            break;

        case ParseState::WAITING_FOR_MSGID1:
            _parsed_msg.msgid[1] = b;
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.state = ParseState::WAITING_FOR_MSGID2;
            break;

        case ParseState::WAITING_FOR_MSGID2:
            _parsed_msg.msgid[2] = b;
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.payload_bytes_received = 0;
            if (_parsed_msg.payload_len > 0) {
                _parsed_msg.state = ParseState::WAITING_FOR_PAYLOAD;
            } else {
                _parsed_msg.state = ParseState::WAITING_FOR_CRC1;
            }
            break;

        case ParseState::WAITING_FOR_PAYLOAD:
            _msg_buff[_msg_buff_len++] = b;
            _parsed_msg.payload_bytes_received++;
            if (_parsed_msg.payload_bytes_received >= _parsed_msg.payload_len) {
                _parsed_msg.state = ParseState::WAITING_FOR_CRC1;
            }
            break;

        case ParseState::WAITING_FOR_CRC1:
            _parsed_msg.crc16 = b;
            _parsed_msg.state = ParseState::WAITING_FOR_CRC2;
            break;

        case ParseState::WAITING_FOR_CRC2:
            _parsed_msg.crc16 |= ((uint16_t)b << 8);

            // check crc over entire packet except the CRC itself
            if (_msg_buff_len >= 2) {
                const uint16_t expected_crc = crc_calculate(_msg_buff, _msg_buff_len);
                if (expected_crc == _parsed_msg.crc16) {
                    // check receiver is FC or broadcast
                    if (_parsed_msg.sysid_rx == AP_MOUNT_K40T_SYSID_FC || _parsed_msg.sysid_rx == 0xFF) {
                        process_packet();
                    }
                } else {
                    debug("crc expected:%x got:%x", (unsigned)expected_crc, (unsigned)_parsed_msg.crc16);
                }
            }
            reset_parser = true;
            break;
        }

        // handle reset of parser
        if (reset_parser) {
            _parsed_msg.state = ParseState::WAITING_FOR_STX;
            _msg_buff_len = 0;
            reset_parser = false;
        }
    }
}

// process successfully decoded packets
void AP_Mount_K40T::process_packet()
{
    // reconstruct msgid
    const uint32_t msgid = ((uint32_t)_parsed_msg.msgid[0]) |
                           (((uint32_t)_parsed_msg.msgid[1]) << 8) |
                           (((uint32_t)_parsed_msg.msgid[2]) << 16);

    const uint8_t *payload = &_msg_buff[AP_MOUNT_K40T_HEADER_LEN];
    const uint8_t payload_len = _parsed_msg.payload_bytes_received;

    switch (msgid) {
    case (uint32_t)MsgID::GIMBAL_STATUS: {
        // 0x000001: gimbal status (2Hz)
        if (payload_len >= 1) {
            // byte0 low nibble: gimbal connection (0=normal), high nibble: camera connection
            // for now just use as heartbeat
        }
        break;
    }

    case (uint32_t)MsgID::GIMBAL_ATTITUDE: {
        // 0x000002: gimbal attitude (10Hz)
        if (payload_len >= 20) {
            // angles are int16 degrees * 100, little-endian
            // yaw joint (bytes 0-1), roll joint (2-3), pitch joint (4-5)
            // yaw attitude (6-7), roll attitude (8-9), pitch attitude (10-11)
            // yaw rate (12-13), pitch rate (14-15), roll rate (16-17)
            const float deg100_to_rad = DEG_TO_RAD * 0.01f;
            const float rate100_to_rads = DEG_TO_RAD * 0.01f;

            _current_angle_rad.z = (int16_t)(payload[0] | (payload[1] << 8)) * deg100_to_rad;
            _current_angle_rad.x = (int16_t)(payload[2] | (payload[3] << 8)) * deg100_to_rad;
            _current_angle_rad.y = (int16_t)(payload[4] | (payload[5] << 8)) * deg100_to_rad;

            // use attitude angles if available (bytes 6-11), else fall back to joint angles
            if (payload_len >= 12) {
                _current_angle_rad.z = (int16_t)(payload[6] | (payload[7] << 8)) * deg100_to_rad;
                _current_angle_rad.x = (int16_t)(payload[8] | (payload[9] << 8)) * deg100_to_rad;
                _current_angle_rad.y = (int16_t)(payload[10] | (payload[11] << 8)) * deg100_to_rad;
            }

            if (payload_len >= 18) {
                _current_rates_rads.z = (int16_t)(payload[12] | (payload[13] << 8)) * rate100_to_rads;
                _current_rates_rads.y = (int16_t)(payload[14] | (payload[15] << 8)) * rate100_to_rads;
                _current_rates_rads.x = (int16_t)(payload[16] | (payload[17] << 8)) * rate100_to_rads;
            }

            _last_attitude_ms = AP_HAL::millis();
        }
        break;
    }

    case (uint32_t)MsgID::CAMERA_SYS_STATUS: {
        // 0x000003: camera system status (1Hz)
        if (payload_len >= 1) {
            _cam_photo_mode = (payload[0] == 0);
        }
        if (payload_len >= 4) {
            _recording = (payload[3] == 1);
        }
        if (payload_len >= 9) {
            _sd_status = payload[8];
        }
        break;
    }

    case (uint32_t)MsgID::IR_CAMERA_STATUS: {
        // 0x000004: IR camera status (5Hz)
        if (payload_len >= 2) {
            _ir_max_temp_c = (int16_t)(payload[0] | (payload[1] << 8));
        }
        if (payload_len >= 4) {
            _ir_min_temp_c = (int16_t)(payload[2] | (payload[3] << 8));
        }
        break;
    }

    case (uint32_t)MsgID::VL_CAMERA_STATUS: {
        // 0x000005: visible light camera status (5Hz)
        if (payload_len >= 1) {
            _zoom_in_progress = (payload[0] == 0x01);
        }
        if (payload_len >= 5) {
            _zoom_x10 = (uint16_t)(payload[3] | (payload[4] << 8));
        }
        break;
    }

    case (uint32_t)MsgID::GPS_INFO_REQ: {
        // 0x000310: camera requests GPS from flight controller
        if (payload_len >= 6) {
            handle_gps_request(payload, payload_len);
        }
        break;
    }

    case (uint32_t)MsgID::LASER_RANGE_PERIODIC: {
        // 0x000406: laser periodic ranging status frame
        if (payload_len >= 4) {
            const uint16_t dist_dm = (uint16_t)(payload[2] | (payload[3] << 8));
            _lrf_dist_m = dist_dm * 0.1f;
            _lrf_valid = true;
            _last_lrf_ms = AP_HAL::millis();
        }
        break;
    }

    default:
        break;
    }
}

// send packet to gimbal/camera
bool AP_Mount_K40T::send_packet(MsgID msg_id, const uint8_t* payload, uint8_t payload_len)
{
    if (_uart == nullptr) {
        return false;
    }

    uint8_t buf[AP_MOUNT_K40T_PACKETLEN_MAX];
    uint8_t len = 0;

    buf[len++] = AP_MOUNT_K40T_STX;
    buf[len++] = payload_len;
    buf[len++] = AP_MOUNT_K40T_SYSID_CAM;   // receiver sysid
    buf[len++] = AP_MOUNT_K40T_COMPID_FC;   // receiver compid
    buf[len++] = _last_seq++;
    buf[len++] = AP_MOUNT_K40T_SYSID_FC;    // sender sysid
    buf[len++] = AP_MOUNT_K40T_COMPID_FC;   // sender compid
    buf[len++] = ((uint32_t)msg_id) & 0xFF;
    buf[len++] = (((uint32_t)msg_id) >> 8) & 0xFF;
    buf[len++] = (((uint32_t)msg_id) >> 16) & 0xFF;

    if (payload_len > 0 && payload != nullptr) {
        memcpy(&buf[len], payload, payload_len);
        len += payload_len;
    }

    const uint16_t crc = crc_calculate(buf, len);
    buf[len++] = crc & 0xFF;
    buf[len++] = (crc >> 8) & 0xFF;

    const bool sent = _uart->write(buf, len) == len;
    if (sent) {
        _last_send_ms = AP_HAL::millis();
    }
    return sent;
}

// handle GPS information request from camera and reply
void AP_Mount_K40T::handle_gps_request(const uint8_t* payload, uint8_t len)
{
    (void)len;
    // payload: byte0=lens_enable, byte1=hours, byte2=minutes, byte3=seconds, byte4-5=milliseconds
    const uint8_t hours = payload[1];
    const uint8_t minutes = payload[2];
    const uint8_t seconds = payload[3];
    const uint16_t milliseconds = (uint16_t)(payload[4] | (payload[5] << 8));

    send_gps_reply(hours, minutes, seconds, milliseconds);
}

void AP_Mount_K40T::send_gps_reply(uint8_t hours, uint8_t minutes, uint8_t seconds, uint16_t milliseconds)
{
    const AP_GPS &gps = AP::gps();
    const AP_AHRS &ahrs = AP::ahrs();

    uint8_t payload[25];
    memset(payload, 0, sizeof(payload));

    // Response code: 0x0000 = success
    payload[0] = 0x00;
    payload[1] = 0x00;

    // Time matching request
    payload[2] = hours;
    payload[3] = minutes;
    payload[4] = seconds;
    payload[5] = milliseconds & 0xFF;
    payload[6] = (milliseconds >> 8) & 0xFF;

    if (gps.status() >= AP_GPS_FixType::FIX_2D) {
        const int32_t lon_deg7 = gps.location().lng;
        const int32_t lat_deg7 = gps.location().lat;
        const int16_t rel_alt_dm = (int16_t)(gps.location().alt * 0.1f); // m to dm
        const int16_t alt_dm = (int16_t)(gps.location().alt * 0.1f);     // m to dm

        payload[7]  = lon_deg7 & 0xFF;
        payload[8]  = (lon_deg7 >> 8) & 0xFF;
        payload[9]  = (lon_deg7 >> 16) & 0xFF;
        payload[10] = (lon_deg7 >> 24) & 0xFF;

        payload[11] = lat_deg7 & 0xFF;
        payload[12] = (lat_deg7 >> 8) & 0xFF;
        payload[13] = (lat_deg7 >> 16) & 0xFF;
        payload[14] = (lat_deg7 >> 24) & 0xFF;

        payload[15] = rel_alt_dm & 0xFF;
        payload[16] = (rel_alt_dm >> 8) & 0xFF;

        payload[17] = alt_dm & 0xFF;
        payload[18] = (alt_dm >> 8) & 0xFF;
    }

    // Aircraft attitude: yaw, roll, pitch in degrees * 100
    const int16_t yaw_cdeg   = (int16_t)(degrees(ahrs.get_yaw_rad()) * 100);
    const int16_t roll_cdeg  = (int16_t)(degrees(ahrs.get_roll_rad()) * 100);
    const int16_t pitch_cdeg = (int16_t)(degrees(ahrs.get_pitch_rad()) * 100);

    payload[19] = yaw_cdeg & 0xFF;
    payload[20] = (yaw_cdeg >> 8) & 0xFF;
    payload[21] = roll_cdeg & 0xFF;
    payload[22] = (roll_cdeg >> 8) & 0xFF;
    payload[23] = pitch_cdeg & 0xFF;
    payload[24] = (pitch_cdeg >> 8) & 0xFF;

    send_packet(MsgID::GPS_INFO_REQ, payload, sizeof(payload));
}

// send target pitch and yaw angles to gimbal
void AP_Mount_K40T::send_target_angles(const MountAngleTarget &angle_rad)
{
    const float pitch_rad = angle_rad.pitch;
    const float yaw_rad = angle_rad.yaw;
    const bool yaw_is_ef = angle_rad.yaw_is_ef;

    // stop if no recent actual angles
    const uint32_t now_ms = AP_HAL::millis();
    if (now_ms - _last_attitude_ms >= AP_MOUNT_K40T_TIMEOUT_MS) {
        return;
    }

    // convert yaw to body-frame
    float yaw_bf_rad = yaw_is_ef ? wrap_PI(yaw_rad - AP::ahrs().get_yaw_rad()) : yaw_rad;

    // enforce body-frame yaw angle limits
    const float yaw_bf_min = radians(_params.yaw_angle_min);
    const float yaw_bf_max = radians(_params.yaw_angle_max);
    yaw_bf_rad = constrain_float(yaw_bf_rad, yaw_bf_min, yaw_bf_max);

    // convert to degrees
    int16_t pitch_deg = (int16_t)roundf(degrees(pitch_rad));
    int16_t yaw_bf_deg = (int16_t)roundf(degrees(yaw_bf_rad));

    // K40T pitch: up 30deg, down 90deg
    // K40T yaw: left 180deg, right 180deg
    pitch_deg = constrain_int16(pitch_deg, -90, 30);
    yaw_bf_deg = constrain_int16(yaw_bf_deg, -180, 180);

    send_gimbal_angle(pitch_deg, yaw_bf_deg);
}

// send gimbal absolute angle command (0x000012)
void AP_Mount_K40T::send_gimbal_angle(int16_t pitch_deg, int16_t yaw_deg)
{
    uint8_t payload[7];

    // pitch
    if (pitch_deg > 0) {
        payload[0] = 0;                 // upward
        payload[1] = pitch_deg & 0xFF;
        payload[2] = (pitch_deg >> 8) & 0xFF;
    } else {
        payload[0] = 1;                 // downward
        const uint16_t abs_pitch = -pitch_deg;
        payload[1] = abs_pitch & 0xFF;
        payload[2] = (abs_pitch >> 8) & 0xFF;
    }

    // yaw (body frame)
    if (yaw_deg < 0) {
        payload[3] = 0;                 // left
        const uint16_t abs_yaw = -yaw_deg;
        payload[4] = abs_yaw & 0xFF;
        payload[5] = (abs_yaw >> 8) & 0xFF;
    } else if (yaw_deg > 0) {
        payload[3] = 1;                 // right
        payload[4] = yaw_deg & 0xFF;
        payload[5] = (yaw_deg >> 8) & 0xFF;
    } else {
        payload[3] = 2;                 // no movement
        payload[4] = 0;
        payload[5] = 0;
    }

    payload[6] = 0; // reserved

    send_packet(MsgID::GIMBAL_ANGLE_CTRL, payload, sizeof(payload));
}

//
// camera controls
//

// take a picture.  returns true on success
bool AP_Mount_K40T::take_picture()
{
    // switch to photo mode first
    if (!send_photo_video_mode(0)) {
        return false;
    }
    return send_take_photo(0x02, true);
}

// start or stop video recording
bool AP_Mount_K40T::record_video(bool start_recording)
{
    // switch to video mode first
    if (!send_photo_video_mode(1)) {
        return false;
    }
    return send_record_video(0x02, start_recording);
}

// set zoom specified as a rate or percentage
bool AP_Mount_K40T::set_zoom(ZoomType zoom_type, float zoom_value)
{
    switch (zoom_type) {
    case ZoomType::RATE: {
        _zoom_type = zoom_type;
        _zoom_rate_target = zoom_value;
        return true;
    }
    case ZoomType::PCT: {
        // convert percentage (0~100) to zoom x10 (1~160)
        const uint16_t zoom_x10 = (uint16_t)constrain_float(linear_interpolate(1, 160, zoom_value, 0, 100), 1, 160);
        if (send_zoom_absolute(zoom_x10)) {
            _zoom_type = zoom_type;
            _zoom_x10 = zoom_x10;
            return true;
        }
        return false;
    }
    }
    return false;
}

// update zoom control (for rate-based zoom)
void AP_Mount_K40T::update_zoom_control()
{
    if (_zoom_type != ZoomType::RATE) {
        return;
    }

    // limit updates to 5hz
    const uint32_t now_ms = AP_HAL::millis();
    if (now_ms - _last_zoom_control_ms < 200) {
        return;
    }
    _last_zoom_control_ms = now_ms;

    if (is_positive(_zoom_rate_target)) {
        send_zoom_continuous(0); // zoom in
    } else if (is_negative(_zoom_rate_target)) {
        send_zoom_continuous(1); // zoom out
    } else {
        send_zoom_continuous(2); // stop
    }
}

// set focus specified as rate, percentage or auto
SetFocusResult AP_Mount_K40T::set_focus(FocusType focus_type, float focus_value)
{
    switch (focus_type) {
    case FocusType::RATE:
        if (is_positive(focus_value)) {
            return send_focus(0x01) ? SetFocusResult::ACCEPTED : SetFocusResult::FAILED; // focus +
        } else if (is_negative(focus_value)) {
            return send_focus(0x02) ? SetFocusResult::ACCEPTED : SetFocusResult::FAILED; // focus -
        } else {
            return send_focus(0x03) ? SetFocusResult::ACCEPTED : SetFocusResult::FAILED; // stop
        }
    case FocusType::PCT:
        return SetFocusResult::UNSUPPORTED;
    case FocusType::AUTO:
        return send_focus(0x00) ? SetFocusResult::ACCEPTED : SetFocusResult::FAILED;
    }
    return SetFocusResult::UNSUPPORTED;
}

// set camera lens as a value from 0 to 5
bool AP_Mount_K40T::set_lens(uint8_t lens)
{
    switch (lens) {
    case 0: // IR
        return send_image_mode((uint8_t)ImageMode::INFRARED);
    case 1: // Visible
        return send_image_mode((uint8_t)ImageMode::VISIBLE);
    case 2: // Split
        return send_image_mode((uint8_t)ImageMode::SPLIT);
    default:
        return false;
    }
}

// set_camera_source is functionally the same as set_lens except primary and secondary lenses are specified by type
bool AP_Mount_K40T::set_camera_source(uint8_t primary_source, uint8_t secondary_source)
{
    (void)secondary_source;
    switch (primary_source) {
    case 0: // Default
        FALLTHROUGH;
    case 1: // RGB
        return send_image_mode((uint8_t)ImageMode::VISIBLE);
    case 2: // IR
        return send_image_mode((uint8_t)ImageMode::INFRARED);
    case 4: // RGB_WIDEANGLE
        return send_image_mode((uint8_t)ImageMode::VISIBLE);
    default:
        return false;
    }
}

// send camera settings message to GCS
void AP_Mount_K40T::send_camera_settings(mavlink_channel_t chan) const
{
    const uint8_t mode_id = _recording ? CAMERA_MODE_VIDEO : CAMERA_MODE_IMAGE;
    const float zoom_pct = linear_interpolate(0, 100, _zoom_x10, 1, 160);

    mavlink_msg_camera_settings_send(
        chan,
        AP_HAL::millis(),
        mode_id,
        zoom_pct,
        NaNf);
}

// send camera capture status message to GCS
void AP_Mount_K40T::send_camera_capture_status(mavlink_channel_t chan) const
{
    const uint8_t image_status = _cam_photo_mode ? 0 : 1;
    const uint8_t video_status = _recording ? 1 : 0;

    mavlink_msg_camera_capture_status_send(
        chan,
        AP_HAL::millis(),
        image_status,
        video_status,
        0,
        0,
        NaNf,
        0);
}

#if AP_MOUNT_SEND_THERMAL_RANGE_ENABLED
// send camera thermal range message to GCS
void AP_Mount_K40T::send_camera_thermal_range(mavlink_channel_t chan) const
{
    const uint32_t now_ms = AP_HAL::millis();
    const bool timeout = now_ms - _last_attitude_ms > AP_MOUNT_K40T_THERM_TIMEOUT_MS;

    mavlink_msg_camera_thermal_range_send(
        chan,
        now_ms,
        _instance + 1,
        _instance + 1,
        timeout ? NaNf : _ir_max_temp_c * 0.1f,
        NaNf,
        NaNf,
        timeout ? NaNf : _ir_min_temp_c * 0.1f,
        NaNf,
        NaNf);
}
#endif

// change camera settings not normally used by autopilot
bool AP_Mount_K40T::change_setting(CameraSetting setting, float value)
{
    switch (setting) {
    case CameraSetting::THERMAL_PALETTE:
        // K40T thermal palette: 1-20
        return send_packet(MsgID::PHOTO_PARAM, nullptr, 0); // not directly supported
    default:
        return false;
    }
}

// get rangefinder distance.  Returns true on success
bool AP_Mount_K40T::get_rangefinder_distance(float& distance_m) const
{
    if (!_lrf_valid) {
        return false;
    }
    const uint32_t now_ms = AP_HAL::millis();
    if (now_ms - _last_lrf_ms > AP_MOUNT_K40T_TIMEOUT_MS) {
        return false;
    }
    distance_m = _lrf_dist_m;
    return true;
}

//
// camera command helpers
//

bool AP_Mount_K40T::send_photo_video_mode(uint8_t mode)
{
    uint8_t payload[2] = { mode, 0 };
    return send_packet(MsgID::PHOTO_VIDEO_MODE, payload, sizeof(payload));
}

bool AP_Mount_K40T::send_take_photo(uint8_t photo_mode, bool start)
{
    uint8_t payload[54] = {};
    payload[0] = photo_mode;
    payload[1] = start ? 0x00 : 0x01;
    return send_packet(MsgID::TAKE_PHOTO, payload, sizeof(payload));
}

bool AP_Mount_K40T::send_record_video(uint8_t video_mode, bool start)
{
    uint8_t payload[54] = {};
    payload[0] = video_mode;
    payload[1] = start ? 0x01 : 0x02;
    return send_packet(MsgID::RECORD_VIDEO, payload, sizeof(payload));
}

bool AP_Mount_K40T::send_zoom_absolute(uint16_t zoom_x10)
{
    uint8_t payload[3];
    payload[0] = 0; // start setting
    payload[1] = zoom_x10 & 0xFF;
    payload[2] = (zoom_x10 >> 8) & 0xFF;
    return send_packet(MsgID::ZOOM_ABSOLUTE, payload, sizeof(payload));
}

bool AP_Mount_K40T::send_zoom_continuous(uint8_t dir)
{
    uint8_t payload[2] = { dir, 0 };
    return send_packet(MsgID::ZOOM_CONTINUOUS, payload, sizeof(payload));
}

bool AP_Mount_K40T::send_focus(uint8_t focus_cmd)
{
    uint8_t payload[10] = {};
    payload[0] = focus_cmd;
    return send_packet(MsgID::FOCUS, payload, sizeof(payload));
}

bool AP_Mount_K40T::send_image_mode(uint8_t mode)
{
    uint8_t payload[2] = { mode, 0 };
    return send_packet(MsgID::IMAGE_MODE, payload, sizeof(payload));
}

// calculate X.25 CRC over buffer
uint16_t AP_Mount_K40T::crc_calculate(const uint8_t *pBuffer, uint16_t length)
{
    auto crc_accumulate = [](uint8_t data, uint16_t *crcAccum) {
        uint8_t tmp = data ^ (*crcAccum & 0xff);
        tmp ^= (tmp << 4);
        *crcAccum = (*crcAccum >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4);
    };

    uint16_t crcTmp = 0xffff;
    while (length--) {
        crc_accumulate(*pBuffer++, &crcTmp);
    }
    return crcTmp;
}

#endif // HAL_MOUNT_K40T_ENABLED
