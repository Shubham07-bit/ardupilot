/*
  K40T Four-Sensor gimbal driver using custom serial protocol

  Protocol reference: K40T Four-Sensor External Protocol Single-channel Streaming Protocol

  Packet format:
  -------------------------------------------------------------------------------
  Field       Index   Bytes    Description
  -------------------------------------------------------------------------------
  STX         0       1        0xFD: start of packet
  Len         1       1        payload length (0-255)
  SysID_rx    2       1        receiver system ID
  CompID_rx   3       1        receiver component ID
  Seq         4       1        packet sequence number
  SysID_tx    5       1        sender system ID
  CompID_tx   6       1        sender component ID
  MsgID       7-9     3        message ID (low, mid, high bytes)
  Payload     10      N        payload data (N = Len)
  CRC16       10+N    2        X.25 CRC over entire packet except CRC itself
  -------------------------------------------------------------------------------
*/

#pragma once

#include "AP_Mount_config.h"

#if HAL_MOUNT_K40T_ENABLED

#include "AP_Mount_Backend_Serial.h"

#include <AP_HAL/AP_HAL.h>
#include <AP_Math/AP_Math.h>
#include <AP_Common/AP_Common.h>

#define AP_MOUNT_K40T_PACKETLEN_MAX     255  // maximum packet size (10 header + 243 payload + 2 crc)
#define AP_MOUNT_K40T_HEADER_LEN        10   // bytes before payload
#define AP_MOUNT_K40T_PAYLOAD_MAX       243  // max payload bytes per spec

class AP_Mount_K40T : public AP_Mount_Backend_Serial
{

public:
    // Constructor
    using AP_Mount_Backend_Serial::AP_Mount_Backend_Serial;

    /* Do not allow copies */
    CLASS_NO_COPY(AP_Mount_K40T);

    // update mount position - should be called periodically
    void update() override;

    // return true if healthy
    bool healthy() const override;

    // return true if this mount accepts roll targets
    bool has_roll_control() const override
    {
        return true;
    }

    // has_pan_control - returns true if this mount can control its pan (required for multicopters)
    bool has_pan_control() const override
    {
        return yaw_range_valid();
    };

    //
    // camera controls
    //

    // take a picture.  returns true on success
    bool take_picture() override;

    // start or stop video recording
    // set start_recording = true to start record, false to stop recording
    bool record_video(bool start_recording) override;

    // set zoom specified as a rate or percentage
    bool set_zoom(ZoomType zoom_type, float zoom_value) override;

    // set focus specified as rate, percentage or auto
    // focus in = -1, focus hold = 0, focus out = 1
    SetFocusResult set_focus(FocusType focus_type, float focus_value) override;

    // set camera lens as a value from 0 to 5
    bool set_lens(uint8_t lens) override;

    // set_camera_source is functionally the same as set_lens except primary and secondary lenses are specified by type
    // primary and secondary sources use the AP_Camera::CameraSource enum cast to uint8_t
    bool set_camera_source(uint8_t primary_source, uint8_t secondary_source) override;

    bool has_camera_information() const override
    {
        return true;
    }
    // return camera vendor name
    void get_camera_vendor_name(char *buf, uint8_t buflen) const override
    {
        strncpy(buf, "K40T", buflen);
    }
    // return camera model name
    void get_camera_model_name(char *buf, uint8_t buflen) const override
    {
        strncpy(buf, "Four-Sensor", buflen);
    }
    // return camera firmware version
    uint32_t get_camera_firmware_version() const override
    {
        return 0;
    }
    // return camera capability flags
    uint32_t get_camera_cap_flags() const override
    {
        return (CAMERA_CAP_FLAGS_CAPTURE_VIDEO |
                CAMERA_CAP_FLAGS_CAPTURE_IMAGE |
                CAMERA_CAP_FLAGS_HAS_BASIC_ZOOM |
                CAMERA_CAP_FLAGS_HAS_BASIC_FOCUS);
    }

    // send camera settings message to GCS
    void send_camera_settings(mavlink_channel_t chan) const override;

    // send camera capture status message to GCS
    void send_camera_capture_status(mavlink_channel_t chan) const override;

#if AP_MOUNT_SEND_THERMAL_RANGE_ENABLED
    // send camera thermal range message to GCS
    void send_camera_thermal_range(mavlink_channel_t chan) const override;
#endif

    // change camera settings not normally used by autopilot
    bool change_setting(CameraSetting setting, float value) override;

    //
    // rangefinder
    //

    // get rangefinder distance.  Returns true on success
    bool get_rangefinder_distance(float& distance_m) const override;

protected:

    // get attitude as a quaternion.  returns true on success
    bool get_attitude_quaternion(Quaternion& att_quat) override;

    // get angular velocity of mount. Only available on some backends
    bool get_angular_velocity(Vector3f& rates) override
    {
        rates = _current_rates_rads;
        return true;
    }

    // K40T only supports angle targets natively
    uint8_t natively_supported_mount_target_types() const override
    {
        return NATIVE_ANGLES_ONLY;
    };

    // send target pitch and yaw angles to gimbal
    void send_target_angles(const MountAngleTarget &angle_rad) override;

private:

    // protocol command ids
    enum class MsgID : uint32_t {
        GIMBAL_STATUS       = 0x000001,
        GIMBAL_ATTITUDE     = 0x000002,
        CAMERA_SYS_STATUS   = 0x000003,
        IR_CAMERA_STATUS    = 0x000004,
        VL_CAMERA_STATUS    = 0x000005,
        GIMBAL_CONTROL      = 0x000010,
        GIMBAL_ANGLE_CTRL   = 0x000012,
        PHOTO_VIDEO_MODE    = 0x000300,
        PHOTO_PARAM         = 0x000301,
        TAKE_PHOTO          = 0x000302,
        RECORD_VIDEO        = 0x000303,
        ZOOM_ABSOLUTE       = 0x000304,
        ZOOM_CONTINUOUS     = 0x000306,
        PRECISE_SHOOT       = 0x000307,
        FOCUS               = 0x000313,
        IMAGE_MODE          = 0x000318,
        LASER_RANGE         = 0x000400,
        LASER_RANGE_PERIODIC= 0x000406,
        GPS_INFO_REQ        = 0x000310,
    };

    // parsing state
    enum class ParseState : uint8_t {
        WAITING_FOR_STX,
        WAITING_FOR_LEN,
        WAITING_FOR_SYSID_RX,
        WAITING_FOR_COMPID_RX,
        WAITING_FOR_SEQ,
        WAITING_FOR_SYSID_TX,
        WAITING_FOR_COMPID_TX,
        WAITING_FOR_MSGID0,
        WAITING_FOR_MSGID1,
        WAITING_FOR_MSGID2,
        WAITING_FOR_PAYLOAD,
        WAITING_FOR_CRC1,
        WAITING_FOR_CRC2,
    };

    // image modes (aka lens selection)
    enum class ImageMode : uint8_t {
        INFRARED    = 0x00,
        VISIBLE     = 0x05,
        SPLIT       = 0x07,
    };

    // reading incoming packets from gimbal/camera
    // results are held in the _parsed_msg structure
    void read_incoming_packets();

    // process successfully decoded packets
    void process_packet();

    // send packet to gimbal/camera
    // returns true on success, false if outgoing serial buffer is full
    bool send_packet(MsgID msg_id, const uint8_t* payload, uint8_t payload_len);

    // calculate X.25 CRC over buffer
    static uint16_t crc_calculate(const uint8_t *pBuffer, uint16_t length);

    // handle GPS information request from camera and reply
    void handle_gps_request(const uint8_t* payload, uint8_t len);
    void send_gps_reply(uint8_t hours, uint8_t minutes, uint8_t seconds, uint16_t milliseconds);

    // camera command helpers
    bool send_photo_video_mode(uint8_t mode); // 0=photo, 1=video
    bool send_take_photo(uint8_t photo_mode, bool start);
    bool send_record_video(uint8_t video_mode, bool start);
    bool send_zoom_absolute(uint16_t zoom_x10);
    bool send_zoom_continuous(uint8_t dir);
    bool send_focus(uint8_t focus_cmd);
    bool send_image_mode(uint8_t mode);

    // gimbal command helpers
    void send_gimbal_angle(int16_t pitch_deg, int16_t yaw_deg);

    // update zoom control (for rate-based zoom)
    void update_zoom_control();

    // internal variables
    uint8_t _msg_buff[AP_MOUNT_K40T_PACKETLEN_MAX];
    uint8_t _msg_buff_len = 0;

    // parser state and unpacked fields
    struct {
        uint8_t payload_len = 0;           // expected number of payload bytes
        uint8_t sysid_rx = 0;              // receiver system id
        uint8_t compid_rx = 0;             // receiver component id
        uint8_t seq = 0;                   // sequence number
        uint8_t sysid_tx = 0;              // sender system id
        uint8_t compid_tx = 0;             // sender component id
        uint8_t msgid[3] = {};             // message id (low, mid, high)
        uint8_t payload_bytes_received = 0;// number of payload bytes received so far
        uint16_t crc16 = 0;                // latest message's crc
        ParseState state = ParseState::WAITING_FOR_STX; // state of incoming message processing
    } _parsed_msg;

    // variables for sending packets
    uint32_t _last_send_ms = 0;        // system time of last packet sent
    uint8_t  _last_seq = 0;            // last sequence number used

    // actual attitude received from gimbal
    Vector3f _current_angle_rad;       // current angles in radians (x=roll, y=pitch, z=yaw)
    Vector3f _current_rates_rads;      // current angular rates in rad/s
    uint32_t _last_attitude_ms = 0;    // system time _current_angle_rad was updated

    // camera state from 0x000003
    bool _cam_photo_mode = true;       // true if in photo mode
    bool _recording = false;           // true if recording video
    uint8_t _sd_status = 0;            // SD card status

    // visible light state from 0x000005
    uint16_t _zoom_x10 = 10;           // current hybrid zoom x10
    bool _zoom_in_progress = false;    // true if zoom is in progress

    // IR state from 0x000004
    int16_t _ir_max_temp_c = 0;        // max temperature (0.1C)
    int16_t _ir_min_temp_c = 0;        // min temperature (0.1C)

    // laser rangefinder
    float _lrf_dist_m = 0;             // distance in meters
    uint32_t _last_lrf_ms = 0;         // system time of last successful LRF read
    bool _lrf_valid = false;           // true if LRF data is valid

    // zoom control
    ZoomType _zoom_type = ZoomType::PCT; // current zoom type
    float _zoom_rate_target = 0;       // current zoom rate target
    uint32_t _last_zoom_control_ms = 0;// system time zoom control was last run
};

#endif // HAL_MOUNT_K40T_ENABLED
