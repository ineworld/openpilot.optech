#pragma once

#include <memory>
#include <string>
#include <optional>

#include <QObject>
#include <QTimer>
#include <QColor>
#include <QRect>
#include <QFuture>
#include <QPolygonF>
#include <QTransform>
#include "nanovg.h"

#include "cereal/messaging/messaging.h"
#include "selfdrive/common/modeldata.h"
#include "selfdrive/common/params.h"
#include "selfdrive/common/timing.h"

const int bdr_s = 20;
const int header_h = 420;
const int footer_h = 280;

const int UI_FREQ = 20;   // Hz
typedef cereal::CarControl::HUDControl::AudibleAlert AudibleAlert;

// TODO: this is also hardcoded in common/transformations/camera.py
// TODO: choose based on frame input size
const float y_offset = Hardware::EON() ? 0.0 : 150.0;
const float ZOOM = Hardware::EON() ? 2138.5 : 2912.8;
typedef struct Rect {
  int x, y, w, h;
  int centerX() const { return x + w / 2; }
  int centerY() const { return y + h / 2; }
  int right() const { return x + w; }
  int bottom() const { return y + h; }
  bool ptInRect(int px, int py) const {
    return px >= x && px < (x + w) && py >= y && py < (y + h);
  }
} Rect;

const vec3 default_face_kpts_3d[] = {
  {-5.98, -51.20, 8.00}, {-17.64, -49.14, 8.00}, {-23.81, -46.40, 8.00}, {-29.98, -40.91, 8.00}, {-32.04, -37.49, 8.00},
  {-34.10, -32.00, 8.00}, {-36.16, -21.03, 8.00}, {-36.16, 6.40, 8.00}, {-35.47, 10.51, 8.00}, {-32.73, 19.43, 8.00},
  {-29.30, 26.29, 8.00}, {-24.50, 33.83, 8.00}, {-19.01, 41.37, 8.00}, {-14.21, 46.17, 8.00}, {-12.16, 47.54, 8.00},
  {-4.61, 49.60, 8.00}, {4.99, 49.60, 8.00}, {12.53, 47.54, 8.00}, {14.59, 46.17, 8.00}, {19.39, 41.37, 8.00},
  {24.87, 33.83, 8.00}, {29.67, 26.29, 8.00}, {33.10, 19.43, 8.00}, {35.84, 10.51, 8.00}, {36.53, 6.40, 8.00},
  {36.53, -21.03, 8.00}, {34.47, -32.00, 8.00}, {32.42, -37.49, 8.00}, {30.36, -40.91, 8.00}, {24.19, -46.40, 8.00},
  {18.02, -49.14, 8.00}, {6.36, -51.20, 8.00}, {-5.98, -51.20, 8.00},
};

struct Alert {
  QString text1;
  QString text2;
  QString type;
  cereal::ControlsState::AlertSize size;
  AudibleAlert sound;

  bool equal(const Alert &a2) {
    return text1 == a2.text1 && text2 == a2.text2 && type == a2.type && sound == a2.sound;
  }

  static Alert get(const SubMaster &sm, uint64_t started_frame) {
    const cereal::ControlsState::Reader &cs = sm["controlsState"].getControlsState();
    if (sm.updated("controlsState")) {
      return {cs.getAlertText1().cStr(), cs.getAlertText2().cStr(),
              cs.getAlertType().cStr(), cs.getAlertSize(),
              cs.getAlertSound()};
    } else if ((sm.frame - started_frame) > 5 * UI_FREQ) {
      const int CONTROLS_TIMEOUT = 5;
      const int controls_missing = (nanos_since_boot() - sm.rcv_time("controlsState")) / 1e9;

      // Handle controls timeout
      if (sm.rcv_frame("controlsState") < started_frame) {
        // car is started, but controlsState hasn't been seen at all
        return {"오픈파일럿을 켤 수 없음", "제어 프로그램 시작 대기 중",
                "controlsWaiting", cereal::ControlsState::AlertSize::MID,
                AudibleAlert::NONE};
      } else if (controls_missing > CONTROLS_TIMEOUT && !Hardware::PC()) {
        // car is started, but controls is lagging or died
        if (cs.getEnabled() && (controls_missing - CONTROLS_TIMEOUT) < 10) {
          return {"즉시 운전하세요", "제어 프로그램 응답 없음",
                  "controlsUnresponsive", cereal::ControlsState::AlertSize::FULL,
                  AudibleAlert::WARNING_IMMEDIATE};
        } else {
          return {"제어 프로그램 응답 없음", "기기를 다시 켜세요",
                  "controlsUnresponsivePermanent", cereal::ControlsState::AlertSize::MID,
                  AudibleAlert::NONE};
        }
      }
    }
    return {};
  }
};

typedef enum UIStatus {
  STATUS_DISENGAGED,
  STATUS_OVERRIDE,
  STATUS_ENGAGED,
  STATUS_WARNING,
  STATUS_ALERT,
  STATUS_CRUISE_STOP,
} UIStatus;

const QColor bg_colors [] = {
  [STATUS_DISENGAGED] =  QColor(0x17, 0x33, 0x49, 0xc8),
  [STATUS_OVERRIDE] = QColor(0x91, 0x9b, 0x95, 0xf1),
  [STATUS_ENGAGED] = QColor(0x17, 0x86, 0x44, 0xf1),
  [STATUS_WARNING] = QColor(0xDA, 0x6F, 0x25, 0xf1),
  [STATUS_ALERT] = QColor(0xC9, 0x22, 0x31, 0xf1),
  [STATUS_CRUISE_STOP] = QColor(0x00, 0x64, 0xC8, 0x96),
};

typedef struct {
  QPointF v[TRAJECTORY_SIZE * 2];
  int cnt;
} line_vertices_data;

typedef struct {
    float x, y;
} vertex_data;

typedef struct {
    float x, y, d, v, y_rel;
} lead_vertex_data;

typedef struct UIScene {
  mat3 view_from_calib;
  cereal::PandaState::PandaType pandaType;

  // modelV2
  float lane_line_probs[4];
  float road_edge_stds[2];
  QPolygonF track_vertices;
  QPolygonF lane_line_vertices[4];
  QPolygonF road_edge_vertices[2];
  QPolygonF lane_barrier_vertices[2];
  QPointF path_end_left_vertices[2];
  QPointF path_end_right_vertices[2];
  float max_distance;

  // lead
  QPointF lead_vertices[2];
  bool lead_radar[2] = {false, false};
  std::vector<lead_vertex_data> lead_vertices_oncoming, lead_vertices_ongoing, lead_vertices_stopped;
  // DMoji state
  float driver_pose_vals[3];
  float driver_pose_diff[3];
  float driver_pose_sins[3];
  float driver_pose_coss[3];
  vec3 face_kpts_draw[std::size(default_face_kpts_3d)];

  float light_sensor, accel_sensor, gyro_sensor;
  bool started, ignition, is_metric, map_on_left, longitudinal_control;
  uint64_t started_frame;
  cereal::DeviceState::Reader deviceState;
  cereal::LongitudinalPlan::Reader longitudinal_plan;

  // GPS summary from the uBlox (ubloxGnss RXM-RAWX + MON-HW): satellites seen, strong ones (C/N0 >= 30 dB-Hz),
  // mean C/N0 of the 4 strongest, antenna short/open. Updated in update_state, zeroed when ubloxGnss stops.
  int gps_sats = 0, gps_strong = 0, gps_cno = 0;
  bool gps_antenna_bad = false;
} UIScene;

class UIState : public QObject {
  Q_OBJECT

public:
  UIState(QObject* parent = 0);
  void updateStatus();
  inline bool worldObjectsVisible() const {
    return sm->rcv_frame("liveCalibration") > scene.started_frame;
  };
  inline bool engaged() const {
    return scene.started && (*sm)["controlsState"].getControlsState().getEnabled();
  };

  int fb_w = 0, fb_h = 0;
  NVGcontext *vg;
  std::map<std::string, int> images;

  std::unique_ptr<SubMaster> sm;

  UIStatus status;
  UIScene scene = {};

  bool awake;
  int prime_type;
  QString language;

  QTransform car_space_transform;
  bool wide_camera;
  bool show_debug = false;
  int show_datetime = 1;
  bool show_tpms = true;
  int show_accel = 2;
  bool show_steer_rotate = true;
  int show_path_end = 1;
  int show_steer_mode = 0;
  bool show_device_stat = true;
  bool show_gps_info = true;
  bool show_conn_info = true;
  int  show_lane_info = 2;
  bool show_blind_spot = true;
  int show_gap_info = 1;
  int show_dm_info = -1;
  int show_radar_info = 0;
  int show_mode = 1;
  float show_z_offset = 1.22;
  int show_path_mode = 0;
  int show_path_color = 14;
  int show_path_mode_cruise_off = 0;
  int show_path_color_cruise_off = 14;
  int show_path_mode_lane = 0;
  int show_path_color_lane = 14;
  float show_path_width = 1.0;
  int show_plot_mode = 0;
  int settings_timeout_s = 180;  // optech '화면 유지 시간' (OptechSettingsTimeout seconds, -1: never)
  int drive_dim = 0;             // optech '주행 중 밝기' (OptechDriveDim %, 0: off)
  bool show_record_btn = false;  // optech '녹화 버튼' (OptechShowRecordBtn)
  bool radar_tracks = false;     // EnableRadarTracks (badge)
  int path_offset = 0;           // PathOffset cm (optech wheel panel)
  bool optech_tune_open = false; // optech wheel panel shown

signals:
  void uiUpdate(const UIState &s);
  void offroadTransition(bool offroad);
  void primeTypeChanged(int prime_type);

private slots:
  void update();

private:
  QTimer *timer;
  bool started_prev = false;
  int prime_type_prev = -1;
};

UIState *uiState();

// device management class

class Device : public QObject {
  Q_OBJECT

public:
  Device(QObject *parent = 0);
  bool settings_open = false;  // the settings window is shown (MainWindow sets it)

private:
  // auto brightness
  const float accel_samples = 5*UI_FREQ;

  bool awake = false;
  int interactive_timeout = 0;
  bool ignition_on = false;
  int last_brightness = 0;
  int idle_frames = 0;    // frames since the last touch ('주행 중 밝기')
  bool dimmed = false;
  FirstOrderFilter brightness_filter;
  QFuture<void> brightness_future;

  void updateBrightness(const UIState &s);
  void updateWakefulness(const UIState &s);
  bool keepApplies() const;
  bool motionTriggered(const UIState &s);
  void setAwake(bool on);

signals:
  void displayPowerChanged(bool on);
  void interactiveTimout();

public slots:
  void resetInteractiveTimout();
  void update(const UIState &s);
};

void ui_update_params(UIState *s);
int get_path_length_idx(const cereal::XYZTData::Reader &line, const float path_height);
void update_model(UIState *s,
                  const cereal::ModelDataV2::Reader &model,
                  const cereal::UiPlan::Reader &plan);
void update_dmonitoring(UIState *s, const cereal::DriverState::Reader &driverstate, float dm_fade_state);
void update_leads(UIState *s, const cereal::RadarState::Reader &radar_state, const cereal::XYZTData::Reader &line);
void update_line_data(const UIState *s, const cereal::XYZTData::Reader &line,
                      float y_off, float z_off, QPolygonF *pvd, int max_idx, bool allow_invert);

// optech 주행 화면 (ShowHudMode 7): tap areas shared by the drawing (paint.cc) and the touch handler (onroad.cc)
inline QRect optech_wheel_rect(int w, int h) { return QRect(w - 30 - 160, h - 20 - 160, 160, 160); }  // bottom-right wheel box (all 3 right boxes this size)
inline QRect optech_tune_panel_rect(int w, int h) { return QRect(w - 30 - 160 - 20 - 440, h - 20 - 160 - 14 - 300, 440, 300); }
inline QRect optech_tune_minus_rect(int w, int h) { QRect p = optech_tune_panel_rect(w, h); return QRect(p.x() + 30, p.y() + 90, 120, 120); }
inline QRect optech_tune_plus_rect(int w, int h) { QRect p = optech_tune_panel_rect(w, h); return QRect(p.right() - 150, p.y() + 90, 120, 120); }
