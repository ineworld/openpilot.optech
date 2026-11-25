#include "selfdrive/ui/paint.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <vector>

//#define __TEST

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define NANOVG_GL3_IMPLEMENTATION
#define nvgCreate nvgCreateGL3
#else
#include <GLES3/gl3.h>
#define NANOVG_GLES3_IMPLEMENTATION
#define nvgCreate nvgCreateGLES3
#endif

#define NANOVG_GLES3_IMPLEMENTATION
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>

#define COLOR_BLACK nvgRGBA(0, 0, 0, 255)
#define COLOR_BLACK_ALPHA(x) nvgRGBA(0, 0, 0, x)
#define COLOR_WHITE nvgRGBA(255, 255, 255, 255)
#define COLOR_WHITE_ALPHA(x) nvgRGBA(255, 255, 255, x)
#define COLOR_RED_ALPHA(x) nvgRGBA(201, 34, 49, x)
#define COLOR_YELLOW nvgRGBA(218, 202, 37, 255)
//#define COLOR_RED nvgRGBA(201, 34, 49, 255)
#define COLOR_RED nvgRGBA(255, 0, 0, 255)
#define COLOR_OCHRE nvgRGBA(218, 111, 37, 255)
#define COLOR_OCHRE_ALPHA(x) nvgRGBA(218, 111, 37, x)
#define COLOR_GREEN nvgRGBA(0, 203, 0, 255)
#define COLOR_GREEN_ALPHA(x) nvgRGBA(0, 153, 0, x)
#define COLOR_BLUE nvgRGBA(0, 0, 255, 255)
#define COLOR_BLUE_ALPHA(x) nvgRGBA(0, 0, 255, x)
#define COLOR_ORANGE nvgRGBA(255, 175, 3, 255)
#define COLOR_ORANGE_ALPHA(x) nvgRGBA(255, 175, 3, x)
#define COLOR_YELLOW_ALPHA(x) nvgRGBA(218, 202, 37, x)
#define COLOR_GREY nvgRGBA(191, 191, 191, 1)

#define BOLD "KaiGenGothicKR-Bold"//"Inter-Bold"//"sans-bold"

int g_fps= 0;
static float g_gps_x = 30;  // GPS line x in the top-left badge row, set by drawLeadApilot after its badges

#if 0
static void ui_print(UIState *s, int x, int y,  const char* fmt, ... )
{
  char* msg_buf = NULL;
  va_list args;
  va_start(args, fmt);
  vasprintf( &msg_buf, fmt, args);
  va_end(args);
  nvgText(s->vg, x, y, msg_buf, NULL);
}
#endif
static void ui_draw_text(const UIState* s, float x, float y, const char* string, float size, NVGcolor color, const char* font_name, float borderWidth=3.0, float shadowOffset=0.0, NVGcolor borderColor=COLOR_BLACK, NVGcolor shadowColor=COLOR_BLACK) {
    y += 6;
    nvgFontFace(s->vg, font_name);
    nvgFontSize(s->vg, size);
    if (borderWidth > 0.0) {
        //NVGcolor borderColor = COLOR_BLACK;
        nvgFillColor(s->vg, borderColor);
        for (int i = 0; i < 360; i += 45) {
            float angle = i * NVG_PI / 180.0f;
            float offsetX = borderWidth * cos(angle);
            float offsetY = borderWidth * sin(angle);
            nvgText(s->vg, x + offsetX, y + offsetY, string, NULL);
        }
    }
    if (shadowOffset != 0.0) {
        //NVGcolor shadowColor = COLOR_BLACK;
        nvgFillColor(s->vg, shadowColor);
        nvgText(s->vg, x + shadowOffset, y + shadowOffset, string, NULL);
    }
    nvgFillColor(s->vg, color);
    nvgText(s->vg, x, y, string, NULL);
}

float a_x = 0.;
float a_y = 0.;
float a_size = 0.;
const int a_max = 100;
char a_font[128]="";
int a_time = -1;
char a_string[256] = "";
NVGcolor a_color = COLOR_WHITE;
static void ui_draw_text_a2(const UIState* s) {
    if (a_time <= 0) return;
    nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
    a_time-=10;
    int a_time1 = a_time;
    if (a_time1 > 100) a_time1 = 100;
    int x = (s->fb_w / 2 * a_time1 + a_x * (a_max - a_time1)) / a_max;
    int y = ((s->fb_h - 400) * a_time1 + a_y * (a_max - a_time1)) / a_max;
    int size = (350 * a_time1 + a_size * (a_max - a_time1)) / a_max;
    if(a_time>=100) ui_draw_text(s, x, y, a_string, size, a_color, a_font, 9.0, 8.0, COLOR_BLACK, COLOR_BLACK);
    else ui_draw_text(s, x, y, a_string, size, a_color, a_font);
}
static void ui_draw_text_a(const UIState* s, float x, float y, const char* string, float size, NVGcolor color, const char* font_name) {
    a_x = x;
    a_y = y;
    strcpy(a_string, string);
    a_size = size;
    a_color = color;
    strcpy(a_font, font_name);
    a_time = 130;
}

static void ui_draw_line(const UIState* s, const QPolygonF& vd, NVGcolor* color, NVGpaint* paint, float stroke=0.0, NVGcolor strokeColor=COLOR_WHITE) {
    if (vd.size() == 0) return;

    nvgBeginPath(s->vg);
    nvgMoveTo(s->vg, vd.at(0).x(), vd.at(0).y());
    for (int i = 1; i < vd.size(); i++) {
        nvgLineTo(s->vg, vd.at(i).x(), vd.at(i).y());
    }
    nvgClosePath(s->vg);
    if (color) {
        nvgFillColor(s->vg, *color);
    }
    else if (paint) {
        nvgFillPaint(s->vg, *paint);
    }
    nvgFill(s->vg);
    if (stroke > 0.0) {
        nvgStrokeColor(s->vg, strokeColor);
        nvgStrokeWidth(s->vg, stroke);
        nvgStroke(s->vg);
    }
}
static void ui_draw_line2(const UIState* s, float x[], float y[], int size, NVGcolor* color, NVGpaint* paint, float stroke=0.0, NVGcolor strokeColor=COLOR_WHITE) {

    nvgBeginPath(s->vg);
    nvgMoveTo(s->vg, x[0], y[0]);
    for (int i = 1; i < size; i++) {
        nvgLineTo(s->vg, x[i], y[i]);
    }
    nvgClosePath(s->vg);
    if (color) {
        nvgFillColor(s->vg, *color);
    }
    else if (paint) {
        nvgFillPaint(s->vg, *paint);
    }
    nvgFill(s->vg);

    if (stroke > 0.0) {
        nvgStrokeColor(s->vg, strokeColor);
        nvgStrokeWidth(s->vg, stroke);
        nvgStroke(s->vg);
    }
}

static void ui_draw_bsd(const UIState* s, const QPolygonF& vd, NVGcolor* color, bool right) {
    int index = vd.length();

    float x[4], y[4];
    for (int i = 0; i < index/2 - 2; i += 2) {

        if (right) {
            x[0] = vd[i + 0].x();
            y[0] = vd[i + 0].y();
            x[1] = vd[i + 1].x();
            y[1] = vd[i + 1].y();
            x[2] = vd[index - i - 3].x();
            y[2] = vd[index - i - 3].y();
            x[3] = vd[index - i - 2].x();
            y[3] = vd[index - i - 2].y();
        }
        else {
            x[0] = vd[i + 0].x();
            y[0] = vd[i + 0].y();
            x[1] = vd[i + 1].x();
            y[1] = vd[i + 1].y();
            x[2] = vd[index - i - 3].x();
            y[2] = vd[index - i - 3].y();
            x[3] = vd[index - i - 2].x();
            y[3] = vd[index - i - 2].y();
        }
        ui_draw_line2(s, x, y, 4, color, nullptr, 3.0f);
    }

}
template <class T>
float interp(float x, std::initializer_list<T> x_list, std::initializer_list<T> y_list, bool extrapolate)
{
    std::vector<T> xData(x_list);
    std::vector<T> yData(y_list);
    int size = xData.size();

    int i = 0;
    if (x >= xData[size - 2]) {
        i = size - 2;
    }
    else {
        while (x > xData[i + 1]) i++;
    }
    T xL = xData[i], yL = yData[i], xR = xData[i + 1], yR = yData[i + 1];
    if (!extrapolate) {
        if (x < xL) yR = yL;
        if (x > xR) yL = yR;
    }

    T dydx = (yR - yL) / (xR - xL);
    return yL + dydx * (x - xL);
}
#if 0
template <class T>
float interp(float x, const T* x_list, const T* y_list, size_t size, bool extrapolate)
{
    int i = 0;
    if (x >= x_list[size - 2]) {
        i = size - 2;
    }
    else {
        while (x > x_list[i + 1]) i++;
    }
    T xL = x_list[i], yL = y_list[i], xR = x_list[i + 1], yR = y_list[i + 1];
    if (!extrapolate) {
        if (x < xL) yR = yL;
        if (x > xR) yL = yR;
    }

    T dydx = (yR - yL) / (xR - xL);
    return yL + dydx * (x - xL);
}
static void ui_draw_path(const UIState* s) {
    const UIScene& scene = s->scene;
    SubMaster& sm = *(s->sm);
    auto plan_position = sm["uiPlan"].getUiPlan().getPosition();
    if (plan_position.getX().size() < 33) {
        plan_position = sm["modelV2"].getModelV2().getPosition();
    }
    float max_distance = std::clamp(plan_position.getX()[TRAJECTORY_SIZE - 1],
        MIN_DRAW_DISTANCE, MAX_DRAW_DISTANCE);

    auto lead_one = sm["radarState"].getRadarState().getLeadOne();
    if (lead_one.getStatus()) {
        const float lead_d = lead_one.getDRel() * 2.;
        max_distance = std::clamp((float)(lead_d - fmin(lead_d * 0.35, 10.)), 0.0f, max_distance);
    }
    auto    car_state = sm["carState"].getCarState();
    float   accel = car_state.getAEgo();
    float   v_ego = car_state.getVEgoCluster();
    float   v_ego_kph = v_ego * MS_TO_KPH;
    const auto line_x = plan_position.getX(), line_y = plan_position.getY(), line_z = plan_position.getZ();
    float idxs[33], line_xs[33], line_ys[33], line_zs[33];
    for (int i = 0; i < 33; i++) {
        idxs[i] = (float)i;
        line_xs[i] = line_x[i];
        line_ys[i] = line_y[i];
        line_zs[i] = line_z[i];
    }

    static bool forward = true;

    if (accel > 0.5) forward = true;
    if (accel < -0.5) forward = false;

    float start_dist = 10.0;
    float dt = 0.01;
    float max_t = sqrt((dist - start_dist) / (v_ego * dt));
    static float pos_t = 2;   // dist = pos_t[] * pos_t[] * v_ego * 0.01 + 3.0,  pos_t = sqrt((dist-3.0) / (v_ego*0.01))
    for (int i = 0; i < 5; i++) {
        dist = (pos_t+i) * (pos_t+i) * v_ego * dt + start_dist;
        if (dist > max_distance) {
            pos_t[i] = 0;
            dist = start_dist;
        }

        if (forward) pos_t[i] = (pos_t[i] + 1) % (int)max_t;
        else if (--pos_t[i] < 0) pos_t[i] = (int)max_t - 1;
    }
    // 0~150M까지 display해준다... 
    // 높이시작은 0.8 ~ s->show_z_offset(max_distance)

}
#endif

void ui_draw_image(const UIState* s, const Rect& r, const char* name, float alpha) {
    nvgBeginPath(s->vg);
    NVGpaint imgPaint = nvgImagePattern(s->vg, r.x, r.y, r.w, r.h, 0, s->images.at(name), alpha);
    nvgRect(s->vg, r.x, r.y, r.w, r.h);
    nvgFillPaint(s->vg, imgPaint);
    nvgFill(s->vg);
}
static void ui_draw_circle_image_rotation(const UIState* s, int center_x, int center_y, int radius, const char* image, NVGcolor color, float img_alpha, float angleSteers = 0) {
    const int img_size = radius * 2.0;
    float img_rotation = angleSteers / 180 * 3.141592;
    int ct_pos = -radius;

    nvgBeginPath(s->vg);
    nvgCircle(s->vg, center_x, center_y, radius);
    nvgFillColor(s->vg, color);
    nvgFill(s->vg);
    //ui_draw_image(s, {center_x - (img_size / 2), center_y - (img_size / 2), img_size, img_size}, image, img_alpha);

    nvgSave(s->vg);
    nvgTranslate(s->vg, center_x, center_y);
    nvgRotate(s->vg, -img_rotation);

    ui_draw_image(s, { ct_pos, ct_pos, img_size, img_size }, image, img_alpha);
    nvgRestore(s->vg);
}
void ui_draw_rect(NVGcontext* vg, const Rect& r, NVGcolor color, int width, float radius) {
    nvgBeginPath(vg);
    radius > 0 ? nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius) : nvgRect(vg, r.x, r.y, r.w, r.h);
    nvgStrokeColor(vg, color);
    nvgStrokeWidth(vg, width);
    nvgStroke(vg);
}
static inline void fill_rect(NVGcontext* vg, const Rect& r, const NVGcolor* color, const NVGpaint* paint, float radius) {
    nvgBeginPath(vg);
    radius > 0 ? nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius) : nvgRect(vg, r.x, r.y, r.w, r.h);
    if (color) nvgFillColor(vg, *color);
    if (paint) nvgFillPaint(vg, *paint);
    nvgFill(vg);
}
void ui_fill_rect(NVGcontext* vg, const Rect& r, const NVGcolor& color, float radius) {
    fill_rect(vg, r, &color, nullptr, radius);
}
void ui_fill_rect(NVGcontext* vg, const Rect& r, const NVGpaint& paint, float radius) {
    fill_rect(vg, r, nullptr, &paint, radius);
}

static NVGcolor get_tpms_color(float tpms) {
    if (tpms < 5 || tpms > 60) // N/A
        return nvgRGBA(255, 255, 255, 220);
    if (tpms < 31)
        return nvgRGBA(255, 90, 90, 220);
    return nvgRGBA(255, 255, 255, 220);
}

static const char *get_tpms_text(float tpms) {
    if (tpms < 5 || tpms > 60)
        return "  -";

    static char str[32];
    snprintf(str, sizeof(str), "%.0f", round(tpms));
    return str;
}

void DrawApilot::drawLaneLines(const UIState* s) {

    static float pathDrawSeq = 0;


    const UIScene& scene = s->scene;
    SubMaster& sm = *(s->sm);
    NVGcolor color;
    auto    car_state = sm["carState"].getCarState();
    bool brake_valid = car_state.getBrakeLights();

    bool left_blindspot = sm["carState"].getCarState().getLeftBlindspot();
    bool right_blindspot = sm["carState"].getCarState().getRightBlindspot();

    // show_lane_info: -1 : all off, 0: path only, 1: path+lane, 2: path+lane+edge
    // lanelines
    if (s->show_lane_info > 0) {
        for (int i = 0; i < std::size(scene.lane_line_vertices); ++i) {
            //color = nvgRGBAf(1.0, 1.0, 1.0, std::clamp<float>(scene.lane_line_probs[i] * 2.0, 0.5, 1.0));
            color = nvgRGBAf(1.0, 1.0, 1.0, (scene.lane_line_probs[i]>0.3)?1.0:0.0);
            ui_draw_line(s, scene.lane_line_vertices[i], &color, nullptr);
        }
    }
    if (s->show_blind_spot) {
#ifdef __TEST
        left_blindspot = right_blindspot  = true;
#endif
        color = nvgRGBA(255, 215, 0, 150);
        // optech: the long bands on the side lanes are replaced by the screen-edge marks (ui_draw_bsd_edge)
        if (false && left_blindspot) ui_draw_bsd(s, scene.lane_barrier_vertices[0], &color, false); // ui_draw_line(s, scene.lane_barrier_vertices[0], &color, nullptr);
        if (false && right_blindspot) ui_draw_bsd(s, scene.lane_barrier_vertices[1], &color, true); // ui_draw_line(s, scene.lane_barrier_vertices[1], &color, nullptr);
    }

    // road edges
    if (s->show_lane_info > 1) {
        for (int i = 0; i < std::size(scene.road_edge_vertices); ++i) {
            //color = nvgRGBAf(1.0, 0.0, 1.0, std::clamp<float>(3.0 - scene.road_edge_stds[i], 0.0, 1.0));
            //color = nvgRGBAf(1.0, 0.0, 1.0, (scene.road_edge_stds[i] < 2.0) ? 1.0 : 0.0);
            float temp_f = std::clamp<float>(scene.road_edge_stds[i] / 2., 0.0, 1.0);
            color = nvgRGBAf(1.0 - temp_f, 0.0, temp_f, 1.0);
            ui_draw_line(s, scene.road_edge_vertices[i], &color, nullptr);
        }
    }
    // paint path
    static bool forward = true;
    int alpha = 120;
    NVGcolor colors[10] = {
         COLOR_RED_ALPHA(alpha),
         nvgRGBA(255,153,0, alpha),
         COLOR_YELLOW_ALPHA(alpha),
         COLOR_GREEN_ALPHA(alpha),
         COLOR_BLUE_ALPHA(alpha),
         nvgRGBA(0,0,128, alpha),
         nvgRGBA(0x8b,0,0xff, alpha),
         COLOR_OCHRE_ALPHA(alpha),
         COLOR_WHITE_ALPHA(alpha),
         COLOR_BLACK_ALPHA(alpha),
    };
    if (s->show_mode == 7) {
        // optech: white see-through path, plus a blue centre line while openpilot steers
        const int n = scene.track_vertices.length();
        if (n >= 4) {
            NVGcolor w = nvgRGBA(255, 255, 255, 46);
            ui_draw_line(s, scene.track_vertices, &w, nullptr);
            if (sm["controlsState"].getControlsState().getEnabled()) {
                float xp[128], yp[128];
                const int glen = std::min(n / 2 - 1, 64);
                const float g = 0.06f;
                for (int i = 0; i < glen; i++) {
                    const int e = n - i - 1, ge = glen * 2 - 1 - i;
                    const float x1 = scene.track_vertices[i].x(), y1 = scene.track_vertices[i].y();
                    const float x2 = scene.track_vertices[e].x(), y2 = scene.track_vertices[e].y();
                    xp[i] = x1 + (x2 - x1) * (0.5f - g); yp[i] = y1 + (y2 - y1) * (0.5f - g);
                    xp[ge] = x1 + (x2 - x1) * (0.5f + g); yp[ge] = y1 + (y2 - y1) * (0.5f + g);
                }
                NVGcolor b = nvgRGBA(55, 138, 221, 235);
                ui_draw_line2(s, xp, yp, glen * 2, &b, nullptr);
            }
        }
        return;
    }
    if (s->show_lane_info > -1) {
        auto lp = sm["lateralPlan"].getLateralPlan();
        auto controls_state = sm["controlsState"].getControlsState();

        int longActiveUser = controls_state.getLongActiveUser();
        int show_path_color = (lp.getUseLaneLines()) ? s->show_path_color_lane : s->show_path_color;
        int show_path_mode = (lp.getUseLaneLines()) ? s->show_path_mode_lane : s->show_path_mode;

        if (longActiveUser <=  0) {
            show_path_color = s->show_path_color_cruise_off;
            show_path_mode = s->show_path_mode_cruise_off;
        }
        if (show_path_mode == 0) {
            ui_draw_line(s, scene.track_vertices, &colors[show_path_color % 10], nullptr,(show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid)?COLOR_RED:COLOR_WHITE);
        }
        else if (show_path_mode >= 13 && show_path_mode <= 15) {
            int     track_vertices_len = scene.track_vertices.length();
            float xp[3][128], yp[3][128];
            float   g = 0.05, gc=0.4;
            switch (show_path_mode) {
            case 13: g = 0.2; gc = 0.10; break;
            case 14: g = 0.45; gc = 0.05; break;
            case 15: 
            default:
                g = 0.05; gc = 0.05; break;
            }
            if (show_path_mode == 15) gc = g;
            int glen = track_vertices_len / 2 - 1;
            for (int i = 0; i < glen; i ++) {
                int e = track_vertices_len - i - 1;
                int ge = glen*2 - 1 - i;
                float x1 = scene.track_vertices[i].x();
                float y1 = scene.track_vertices[i].y();
                float x2 = scene.track_vertices[e].x();
                float y2 = scene.track_vertices[e].y();
                xp[0][i] = x1;  yp[0][i] = y1;
                xp[0][ge] = x1 + (x2 - x1) * g; yp[0][ge] = y1 + (y2 - y1) * g;
                xp[1][i] = x1 + (x2 - x1) * (0.5 - gc); yp[1][i] = y1 + (y2 - y1) * (0.5 - gc);
                xp[1][ge] = x1 + (x2 - x1) * (0.5 + gc); yp[1][ge] = y1 + (y2 - y1) * (0.5 + gc);
                xp[2][i] = x1 + (x2 - x1) * (1.0 - g); yp[2][i] = y1 + (y2 - y1) * (1.0 - g);
                xp[2][ge] = x2; yp[2][ge] = y2;
            }
            if(show_path_mode==13 || show_path_mode == 14)
                ui_draw_line2(s, xp[0], yp[0], glen*2, &colors[show_path_color % 10], nullptr, (show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid) ? COLOR_RED : COLOR_WHITE);
            if(show_path_mode==13 || show_path_mode == 15)
                ui_draw_line2(s, xp[1], yp[1], glen * 2, &colors[show_path_color % 10], nullptr, (show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid) ? COLOR_RED : COLOR_WHITE);
            if (show_path_mode == 13 || show_path_mode == 14)
                ui_draw_line2(s, xp[2], yp[2], glen * 2, &colors[show_path_color % 10], nullptr, (show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid) ? COLOR_RED : COLOR_WHITE);
        }
        else if (show_path_mode >= 9) {
            int     track_vertices_len = scene.track_vertices.length();
            //printf("len = %d\n", track_vertices_len);
            float   x[6], y[6];
            int     color_n = 0;
            //for (int i = 0; i < track_vertices_len / 2; i++)
            //    printf("(%.1f,%.1f)(%.1f,%.1f)\n", scene.track_vertices[i].x(), scene.track_vertices[i].y(), scene.track_vertices[track_vertices_len-i - 1].x(), scene.track_vertices[track_vertices_len-i - 1].y());
            for (int i = 0; i < track_vertices_len / 2 - 1; i += 3) {
                int e = track_vertices_len - i - 1;
                x[0] = scene.track_vertices[i].x();
                y[0] = scene.track_vertices[i].y();
                x[1] = scene.track_vertices[i + 1].x();
                y[1] = scene.track_vertices[i + 1].y();
                x[2] = (scene.track_vertices[i + 2].x() + scene.track_vertices[e - 2].x()) / 2;
                y[2] = (scene.track_vertices[i + 2].y() + scene.track_vertices[e - 2].y()) / 2;
                x[3] = scene.track_vertices[e - 1].x();
                y[3] = scene.track_vertices[e - 1].y();
                x[4] = scene.track_vertices[e].x();
                y[4] = scene.track_vertices[e].y();
                x[5] = (x[1] + x[3]) / 2;
                y[5] = (y[1] + y[3]) / 2;
                //ui_draw_line2(s, x, y, 6, &colors[color_n], nullptr, (show_path_color >= 10) ? 2.0 : 0.0);
                ui_draw_line2(s, x, y, 6, &colors[show_path_color % 10], nullptr, (show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid) ? COLOR_RED : COLOR_WHITE);

                if (++color_n > 6) color_n = 0;
            }
        }
        else {

            int     track_vertices_len = scene.track_vertices.length();
            //color = nvgRGBA(0, 150, 0, 30);
            float   accel = car_state.getAEgo();
            float   v_ego = car_state.getVEgoCluster();
            float   v_ego_kph = v_ego * MS_TO_KPH;
#ifdef __TEST
            v_ego_kph = 20.0;
            accel = -1.2;
#endif
            float   seq = (1.0 * v_ego_kph / 100.0);
            if (seq < 0.3) seq = 0.3;
            if (accel < -1.0) forward = false;
            if (accel > -0.5) forward = true;
            int max_seq = track_vertices_len / 4 + 3;
#ifdef __TEST
            if(max_seq> 16) max_seq = 16;
#endif
            static int pathDrawSeq2 = -1;

            if (forward) {
                pathDrawSeq += seq;
                if (pathDrawSeq > max_seq) {
                    pathDrawSeq = (pathDrawSeq2>=0) ? pathDrawSeq2: 0;
                }
            }
            else {
                pathDrawSeq -= seq;
                if (pathDrawSeq < 0) {
                    pathDrawSeq = (pathDrawSeq2>=0)? pathDrawSeq2: max_seq;
                }
            }
            if (max_seq > 15) pathDrawSeq2 = ((int)pathDrawSeq - max_seq/2 + max_seq) % max_seq;
            else pathDrawSeq2 = -5;

            float   x[6], y[6];

            switch (show_path_mode) {
            case 1: case 2: case 5: case 6:
                for (int i = 0, color_n = 0; i < track_vertices_len / 2 - 4; i += 2) {
                    x[0] = scene.track_vertices[i].x();
                    y[0] = scene.track_vertices[i].y();
                    x[1] = scene.track_vertices[i + 2].x();
                    y[1] = scene.track_vertices[i + 2].y();
                    x[2] = scene.track_vertices[track_vertices_len - i - 3].x();
                    y[2] = scene.track_vertices[track_vertices_len - i - 3].y();
                    x[3] = scene.track_vertices[track_vertices_len - i - 1].x();
                    y[3] = scene.track_vertices[track_vertices_len - i - 1].y();

                    int draw = false;
                    if ((int)pathDrawSeq == i / 2 || (int)pathDrawSeq == i / 2 - 2) draw = true;
                    if (pathDrawSeq2 == i / 2 || pathDrawSeq2 == i / 2 - 2) draw = true;
                    //if ((int)(pathDrawSeq + 0.5) * 2 == i || (((int)(pathDrawSeq + 0.5) - 2) * 2 == i))  draw = true;
                    if (track_vertices_len / 2 < 8) draw = true;
                    if (show_path_mode == 5 || show_path_mode == 6) draw = true;

                    if (draw) {
                        switch (show_path_mode) {
                        case 2: case 6: ui_draw_line2(s, x, y, 4, &colors[color_n], nullptr, (show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid) ? COLOR_RED : COLOR_WHITE); break;
                        default:        ui_draw_line2(s, x, y, 4, &colors[show_path_color % 10], nullptr, (show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid) ? COLOR_RED : COLOR_WHITE); break;
                        }
                    }

                    if (i > 1) color_n++;
                    if (color_n > 6) color_n = 0;
                }
                break;
            case 3: case 4: case 7: case 8:
                for (int i = 0, color_n = 0; i < track_vertices_len / 2 - 4; i += 2) {
                    x[0] = scene.track_vertices[i].x();
                    y[0] = scene.track_vertices[i].y();
                    x[1] = scene.track_vertices[i + 2].x();
                    y[1] = scene.track_vertices[i + 2].y();
                    x[2] = (scene.track_vertices[i + 4].x() + scene.track_vertices[track_vertices_len - i - 5].x()) / 2;
                    y[2] = (scene.track_vertices[i + 4].y() + scene.track_vertices[track_vertices_len - i - 5].y()) / 2;
                    x[3] = scene.track_vertices[track_vertices_len - i - 3].x();
                    y[3] = scene.track_vertices[track_vertices_len - i - 3].y();
                    x[4] = scene.track_vertices[track_vertices_len - i - 1].x();
                    y[4] = scene.track_vertices[track_vertices_len - i - 1].y();
                    x[5] = (x[1] + x[3]) / 2;
                    y[5] = (y[1] + y[3]) / 2;

                    int draw = false;
                    if ((int)pathDrawSeq == i / 2 || (int)pathDrawSeq == i / 2 - 2) draw = true;
                    if (pathDrawSeq2 == i / 2 || pathDrawSeq2 == i / 2 - 2) draw = true;
                    //if ((int)(pathDrawSeq + 0.5) * 2 == i || (((int)(pathDrawSeq + 0.5) - 2) * 2 == i))  draw = true;
                    if (track_vertices_len / 2 < 8) draw = true;
                    if (show_path_mode == 7 || show_path_mode == 8) draw = true;

                    if (draw) {
                        switch (show_path_mode) {
                        case 4: case 8:     ui_draw_line2(s, x, y, 6, &colors[color_n], nullptr, (show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid) ? COLOR_RED : COLOR_WHITE); break;
                        default:            ui_draw_line2(s, x, y, 6, &colors[show_path_color % 10], nullptr, (show_path_color >= 10 || brake_valid) ? 2.0 : 0.0, (brake_valid) ? COLOR_RED : COLOR_WHITE); break;

                        }
                    }
                    if (i > 1) color_n++;
                    if (color_n > 6) color_n = 0;
                }
                break;
            }
        }
    }
}

#define PLOT_MAX 500

int     plotSize = 0;
int     plotIndex = 0;
float   plotQueue[2][PLOT_MAX];
float   plotMin = 0.;
float   plotMax = 0.;
float   plotShift = 0.0;
float   plotX = 300.0;
float   plotWidth = 1000;
float   plotY = 30.0;
float   plotHeight = 300.0;
float   plotRatio = 1.0;
int     show_plot_mode_prev = -1;
static void ui_draw_plotting(const UIState* s, int start, float x, float y[], int size, NVGcolor* color, float stroke = 0.0) {

    nvgBeginPath(s->vg);
    plotRatio = (plotMax - plotMin) < 1.0 ? plotHeight : plotHeight / (plotMax - plotMin);
    float dx = 2.0;
    char str[128];

    for (int i = 0; i < size; i++) {
        float data = y[(start - i + PLOT_MAX) % PLOT_MAX];
        float plot_y = plotY + plotHeight - (data - plotMin) * plotRatio;
        if (i == 0) {
            nvgMoveTo(s->vg, x + (size - i)*dx, plot_y);
            sprintf(str, "%.1f", data);
            ui_draw_text(s, x + (size - i)*dx + 50, plot_y + 40, str, 40, *color, BOLD);
        }
        else nvgLineTo(s->vg, x + (size - i)*dx, plot_y);
    }
    //nvgClosePath(s->vg);

    if (stroke > 0.0) {
        nvgStrokeColor(s->vg, *color);
        nvgStrokeWidth(s->vg, stroke);
        nvgStroke(s->vg);
    }
}

// 에잉 시간텀... ㅠㅠ
static void make_plot_data(const UIState* s, float& data1, float& data2) {
    SubMaster& sm = *(s->sm);
    auto    car_state = sm["carState"].getCarState();
    float   a_ego = car_state.getAEgo();
    float   v_ego = car_state.getVEgo();
    //auto    car_control = sm["carControl"].getCarControl();
    //float   accel = car_control.getActuators().getAccel();
    auto    live_parameters = sm["liveParameters"].getLiveParameters();
    float   roll = live_parameters.getRoll();
    auto    controls_state = sm["controlsState"].getControlsState();
    float   curvature = controls_state.getCurvature();
    //float   desired_curvature = controls_state.getDesiredCurvature();
    const auto lp = sm["longitudinalPlan"].getLongitudinalPlan();
    float   accel = lp.getAccels()[0];
    float   speeds_0 = lp.getSpeeds()[0];
    const auto lat_plan = sm["lateralPlan"].getLateralPlan();
    float   curvatures_0 = lat_plan.getCurvatures()[0];

    const cereal::ModelDataV2::Reader& model = sm["modelV2"].getModelV2();
    const auto position = model.getPosition();
    const auto velocity = model.getVelocity();

    auto lead_radar = sm["radarState"].getRadarState().getLeadOne();

    switch (s->show_plot_mode) {
    case 0:
    case 1:
        data1 = a_ego;
        data2 = accel;
        break;
    case 2:
        // curvature * v * v : 원심가속도
        data1 = (curvature * v_ego * v_ego) - (roll * 9.81);
        //data2 = (desired_curvature * v_ego * v_ego) - (roll * 9.81);
        data2 = (curvatures_0 * v_ego * v_ego) - (roll * 9.81);
        break;
    case 3:
        data1 = v_ego;
        data2 = speeds_0;
        break;
    case 4:
        data1 = position.getX()[32];
        data2 = velocity.getX()[32];
        break;
    case 5:
        data1 = lead_radar.getVLeadK();
        data2 = lead_radar.getALeadK();
        break;
    default:
        data1 = data2 = 0;
        break;
    }
    if (s->show_plot_mode != show_plot_mode_prev) {
        plotSize = 0;
        plotIndex = 0;
        plotMin = 0.;
        plotMax = 0.;
        show_plot_mode_prev = s->show_plot_mode;
    }
}

void ui_draw_plot(const UIState* s) {

    if (s->show_plot_mode == 0) return;

    float _data = 0.;
    float _data1 = 0.;

    make_plot_data(s, _data, _data1);

#ifdef __TEST
    static float _data_s = 0.0;
    _data = _data_s;
    _data += 0.1;
    if (_data > 2.0) _data = -2.0;
    _data1 = 3. - _data;

    _data_s = _data;
#endif
    if (plotMin > _data) plotMin = _data;
    if (plotMax < _data) plotMax = _data;
    if (plotMin > _data1) plotMin = _data1;
    if (plotMax < _data1) plotMax = _data1;
    plotIndex = (plotIndex + 1) % PLOT_MAX;
    plotQueue[0][plotIndex] = _data;
    plotQueue[1][plotIndex] = _data1;
    if (plotSize < PLOT_MAX - 1) plotSize++;

    if (s->fb_w < 1200) return;

    NVGcolor color[2] = { COLOR_YELLOW, COLOR_GREEN };
    for (int i = 0; i < 2; i++) {
        //ui_draw_plotting(s, i, plotX, plotQueue[i], plotSize, &color[i], nullptr);
        ui_draw_plotting(s, plotIndex, plotX, plotQueue[i], plotSize, &color[i], 3.0f);
    }
    //ui_draw_rect(s->vg, { (int)plotX, (int)plotY, (int)plotWidth, (int)plotHeight }, COLOR_WHITE, 2, 0);

}
void ui_draw_radar_info(const UIState* s) {

    char str[128];

    if (s->show_radar_info) {
        const auto lead_one = (*s->sm)["radarState"].getRadarState().getLeadOne();
        const bool skip_lead_one = (s->show_mode == 7) && lead_one.getStatus();
        const QPointF& lv = s->scene.lead_vertices[0];
        auto is_primary_lead = [&](float rx, float ry, float rd, float ry_rel) {
            if (!skip_lead_one) return false;
            if (std::fabs(rd - lead_one.getDRel()) < 4.0f && std::fabs(ry_rel - lead_one.getYRel()) < 1.4f) return true;
            if (std::fabs(rx - (float)lv.x()) < 110.f && std::fabs(ry - (float)lv.y()) < 110.f) return true;
            return false;
        };
        bool disp = false;
        int wStr = 40;
        if (s->show_radar_info >= 3) {
            for (auto const& vrd : s->scene.lead_vertices_stopped) {
                auto [rx, ry, rd, rv, ry_rel] = vrd;
                if (is_primary_lead(rx, ry, rd, ry_rel)) continue;
                // stopped object: an amber box saying "정지", shaped like the moving ones (user 2026-11-23: the
                // small '*' was hard to see)
                const int bw = 84;
                ui_fill_rect(s->vg, { (int)rx - bw / 2, (int)ry - 35, bw, 42 }, nvgRGBA(239, 159, 39, 255), 15);
                ui_draw_text(s, rx, ry - 1, "정지", 34, COLOR_WHITE, BOLD);
                if (s->show_radar_info >= 2) {
                    sprintf(str, "%.1f", ry_rel);
                    ui_draw_text(s, rx, ry - 40, str, 30, COLOR_WHITE, BOLD);
                }
            }
        }
        for (auto const& vrd : s->scene.lead_vertices_ongoing) {
            auto [rx, ry, rd, rv, ry_rel] = vrd;
            if (is_primary_lead(rx, ry, rd, ry_rel)) continue;
            disp = true;
            sprintf(str, "%.0f", rv * 3.6);
            wStr = 35 * (strlen(str) + 0);
            ui_fill_rect(s->vg, { (int)(rx - wStr / 2), (int)(ry - 35), wStr, 42 }, COLOR_GREEN, 15);
            ui_draw_text(s, rx, ry, str, 40, COLOR_WHITE, BOLD);
            if (s->show_radar_info >= 2) {
                sprintf(str, "%.1f", ry_rel);
                ui_draw_text(s, rx, ry - 40, str, 30, COLOR_WHITE, BOLD);
            }
        }
        for (auto const& vrd : s->scene.lead_vertices_oncoming) {
            auto [rx, ry, rd, rv, ry_rel] = vrd;
            if (is_primary_lead(rx, ry, rd, ry_rel)) continue;
            sprintf(str, "%.0f", rv * 3.6);
            wStr = 35 * (strlen(str) + 0);
            ui_fill_rect(s->vg, { (int)rx - wStr / 2, (int)ry - 35, wStr, 42 }, COLOR_RED, 15);
            ui_draw_text(s, rx, ry, str, 40, COLOR_WHITE, BOLD);
            if (s->show_radar_info >= 2) {
                sprintf(str, "%.1f", ry_rel);
                ui_draw_text(s, rx, ry - 40, str, 30, COLOR_WHITE, BOLD);
            }
        }
    }
}
float filter_x = 0.0;
float filter_y = 0.0;

// ---- carrot(C3X) style lower-left HUD box (ShowHudMode=6) ----
// Layout follows carrot-wip selfdrive/ui/onroad/hud_renderer.py (_draw_set_speed_carrot),
// anchored at (bx, by) = speed text position (140, fb_h - 230).
static int carrot_blink = 0;  // 0..15, "on" half when <= 8

static void carrot_round_box(const UIState* s, float x, float y, float w, float h, NVGcolor fill, NVGcolor line, float radius, float line_width) {
    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, x, y, w, h, radius);
    nvgFillColor(s->vg, fill);
    nvgFill(s->vg);
    if (line_width > 0.0f) {
        nvgStrokeColor(s->vg, line);
        nvgStrokeWidth(s->vg, line_width);
        nvgStroke(s->vg);
    }
}

static const char* carrot_gear_text(cereal::CarState::GearShifter gear) {
    switch (gear) {
    case cereal::CarState::GearShifter::PARK: return "P";
    case cereal::CarState::GearShifter::DRIVE: return "D";
    case cereal::CarState::GearShifter::NEUTRAL: return "N";
    case cereal::CarState::GearShifter::REVERSE: return "R";
    case cereal::CarState::GearShifter::SPORT: return "S";
    case cereal::CarState::GearShifter::LOW: return "L";
    case cereal::CarState::GearShifter::BRAKE: return "B";
    case cereal::CarState::GearShifter::ECO: return "E";
    case cereal::CarState::GearShifter::UNKNOWN: return "U";
    default: return "M";
    }
}

// background box + CPU/MEM/DISK tiles (drawn before the speed text)
static void carrot_hud_background(const UIState* s, int bx, int by, bool cam_detected) {
    const SubMaster& sm = *(s->sm);
    nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
    carrot_blink = (carrot_blink + 1) % 16;
    const bool show_dev = s->show_device_stat;
    NVGcolor bg = (cam_detected && carrot_blink > 8) ? nvgRGBA(255, 0, 0, 180) : nvgRGBA(0, 0, 0, 90);
    if (show_dev) carrot_round_box(s, bx - 120, by - 270, 475, 495, bg, COLOR_WHITE, 15, 2);
    else          carrot_round_box(s, bx - 120, by - 130, 475, 355, bg, COLOR_WHITE, 15, 2);
    if (!show_dev) return;

    auto deviceState = sm["deviceState"].getDeviceState();
    const auto cpuTempC = deviceState.getCpuTempC();
    float cpuTemp = 0.f;
    if (std::size(cpuTempC) > 0) {
        for (int i = 0; i < std::size(cpuTempC); i++) cpuTemp += cpuTempC[i];
        cpuTemp /= (float)std::size(cpuTempC);
    }
    const auto cpuUsage = deviceState.getCpuUsagePercent();
    int cpuUse = 0;
    if (std::size(cpuUsage) > 0) {
        for (int i = 0; i < std::size(cpuUsage); i++) cpuUse += cpuUsage[i];
        cpuUse /= (int)std::size(cpuUsage);
    }
    const NVGcolor tile_ok = nvgRGBA(0, 255, 0, 190);
    const NVGcolor tile_hot = nvgRGBA(255, 0, 0, 255);
    const bool blink_on = carrot_blink <= 8;
    char str[32];

    const int dx = bx - 35, dy = by - 200;
    const int xs[3] = {dx, dx + 150, dx + 300};
    // CPU temperature (the font has no degree sign, so the unit is in the label), CPU use (core average), screen FPS
    const char* labels[3] = {"CPU온도", "CPU사용", "FPS"};
    NVGcolor fills[3] = {(cpuTemp > 80 && blink_on) ? tile_hot : tile_ok, (cpuUse > 90 && blink_on) ? tile_hot : tile_ok,
                         (g_fps > 0 && g_fps < 15 && blink_on) ? tile_hot : tile_ok};
    for (int i = 0; i < 3; i++) {
        carrot_round_box(s, xs[i] - 65, dy - 38, 130, 90, fills[i], COLOR_WHITE, 7, 2);
        if (i == 0) snprintf(str, sizeof(str), "%.0f", cpuTemp);
        else if (i == 1) snprintf(str, sizeof(str), "%d%%", cpuUse);
        else snprintf(str, sizeof(str), "%d", g_fps);
        ui_draw_text(s, xs[i], dy - 5, labels[i], 25, COLOR_WHITE, BOLD, 1.0f, 4.0f);
        ui_draw_text(s, xs[i], dy + 40, str, 40, COLOR_WHITE, BOLD, 1.0f, 4.0f);
    }
}

// driving mode, gap number/bars, gear and speed-limit tile (drawn after the speed text)
static void carrot_hud_lower(const UIState* s, int bx, int by, int my_driving_mode, int gap,
                             int cam_limit, int road_limit, float cur_speed, int active_nda) {
    // last shown values: a change pops the new value from the screen center (ui_draw_text_a), like c3-wip
    static int mode_last = -1, gap_last = -1;
    static char gear_last[8] = "";
    SubMaster& sm = *(s->sm);
    nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
    const bool blink_on = carrot_blink <= 8;
    char str[32];

    // driving mode (+ GPS when fixed)
    const char* mode_text = nullptr;
    NVGcolor mode_color = nvgRGBA(255, 255, 255, 200);
    switch (my_driving_mode) {
    case 1: mode_text = "연비"; mode_color = nvgRGBA(0, 255, 0, 200); break;
    case 2: mode_text = "안전"; mode_color = nvgRGBA(255, 165, 0, 200); break;
    case 3: mode_text = "일반"; mode_color = nvgRGBA(128, 128, 128, 210); break;
    case 4: mode_text = "고속"; mode_color = nvgRGBA(255, 0, 0, 200); break;
    }
    if (mode_text) {
        const int mx = bx - 50, my = by + 175;
        carrot_round_box(s, mx - 55, my - 38, 110, 48, mode_color, COLOR_WHITE, 6, 2);
        ui_draw_text(s, mx, my - 2, mode_text, 32, COLOR_WHITE, BOLD, 2.0f, 4.0f);
        if (mode_last != -1 && mode_last != my_driving_mode) ui_draw_text_a(s, mx, my, mode_text, 30, COLOR_WHITE, BOLD);
        // GPS speed (the dashboard speed reads a few % high): green with a fix, gray "GPS --" while searching
        if (sm.alive("gpsLocationExternal")) {
            const auto gps = sm["gpsLocationExternal"].getGpsLocationExternal();
            if (gps.getFlags() & 1) {
                const float gps_speed = gps.getSpeed() * (s->scene.is_metric ? MS_TO_KPH : MS_TO_MPH);
                snprintf(str, sizeof(str), "GPS %d", (int)(std::max(gps_speed, 0.0f) + 0.5f));
                ui_draw_text(s, mx, my - 45, str, 30, nvgRGBA(0, 255, 0, 255), BOLD, 2.0f, 4.0f);
            } else {
                ui_draw_text(s, mx, my - 45, "GPS --", 30, nvgRGBA(170, 170, 170, 255), BOLD, 2.0f, 4.0f);
            }
        }
    }

    // gap number + bars
    snprintf(str, sizeof(str), "%d", gap);
    ui_draw_text(s, bx + 220, by + 77, str, 40, COLOR_WHITE, BOLD, 2.0f, 4.0f);
    if (gap_last != -1 && gap_last != gap) ui_draw_text_a(s, bx + 220, by + 77, str, 40, COLOR_WHITE, BOLD);
    const float ddy = 80.0f / 4.0f;
    for (int i = 0; i < std::clamp(gap, 0, 4); i++) {
        carrot_round_box(s, bx + 270, by + 185 - ddy * (i + 1) + 2, 70, ddy - 2, nvgRGBA(0, 255, 0, 210), COLOR_WHITE, 1, 2);
    }

    // gear
    const int gx = bx + 305, gy = by + 60;
    carrot_round_box(s, gx - 35, gy - 70, 70, 80, nvgRGBA(0, 255, 0, 210), COLOR_WHITE, 7, 3);
    const char* gear = carrot_gear_text(sm["carState"].getCarState().getGearShifter());
    ui_draw_text(s, gx, gy + 5, gear, 70, COLOR_WHITE, BOLD, 2.0f, 4.0f);
    if (gear_last[0] && strcmp(gear, gear_last)) ui_draw_text_a(s, gx, gy + 5, gear, 70, COLOR_WHITE, BOLD);
    snprintf(gear_last, sizeof(gear_last), "%s", gear);
    mode_last = my_driving_mode;
    gap_last = gap;

    // navigation link: APN (green) / APM (blue), between the limit tile and the gap bars
    const int nx = bx + 200, ny = by + 175;
    if (active_nda >= 200) {
        carrot_round_box(s, nx - 55, ny - 38, 110, 48, nvgRGBA(0, 203, 0, 255), COLOR_WHITE, 6, 2);
        ui_draw_text(s, nx, ny, "APN", 40, COLOR_WHITE, BOLD, 2.0f, 4.0f);
    }
    else if (active_nda >= 100) {
        carrot_round_box(s, nx - 55, ny - 38, 110, 48, nvgRGBA(0, 120, 255, 210), COLOR_WHITE, 6, 2);
        ui_draw_text(s, nx, ny, "APM", 40, COLOR_WHITE, BOLD, 2.0f, 4.0f);
    }

    // speed limit: CAM (blinking red/yellow) or road LIMIT (red when over)
    const int lx = bx + 75, ly = by + 175;
    const char* label = "LIMIT";
    int disp = (cam_limit > 0) ? cam_limit : road_limit;  // km/h
    if (!s->scene.is_metric) disp = (int)(disp * KM_TO_MILE + 0.5);
    // cur_speed is already in display units
    NVGcolor lcolor = (disp > 0 && cur_speed > disp + 2) ? nvgRGBA(255, 0, 0, 210) : nvgRGBA(255, 255, 255, 210);
    if (cam_limit > 0) {
        label = "CAM";
        lcolor = blink_on ? nvgRGBA(255, 0, 0, 210) : nvgRGBA(255, 255, 0, 210);
    }
    ui_draw_text(s, lx, ly - 45, label, 30, COLOR_WHITE, BOLD, 2.0f, 4.0f);
    carrot_round_box(s, lx - 55, ly - 38, 110, 48, lcolor, COLOR_WHITE, 6, 2);
    snprintf(str, sizeof(str), "%d", disp);
    ui_draw_text(s, lx, ly, str, 40, COLOR_WHITE, BOLD, 2.0f, 4.0f);
}

// ===================================================================================================================
// optech 주행 화면 (ShowHudMode 7, the only screen since 2026-11-23). Layout agreed with the user 2026-11-23
// (D:/Workspace Claude/optech_주행화면_개편_논의.md): time + badges, road limit + camera sign, set speed box,
// GPS/gear box on the left; big speed with traffic light, turn signal arrows and turn guidance at the top centre;
// lead distance / lead speed / time gap under the lead; steering wheel (tap: path offset panel), CPU and tyres on
// the right. The old carrot box and target are not drawn in this mode.
// ===================================================================================================================
static void optech_text(const UIState* s, float x, float y, const char* str, float size, NVGcolor color, int align) {
    nvgTextAlign(s->vg, align);
    ui_draw_text(s, x, y, str, size, color, BOLD, 2.0f, 0.0f);
}

// text whose letters sit vertically centred on cy (digits and Hangul both reach about 0.72 size above the baseline;
// ui_draw_text adds 6). halign: NVG_ALIGN_LEFT / CENTER / RIGHT
static void optech_text_vc(const UIState* s, float x, float cy, const char* str, float size, NVGcolor color, int halign) {
    optech_text(s, x, cy + 0.36f * size - 6, str, size, color, halign | NVG_ALIGN_BASELINE);
}

static void optech_chevron(const UIState* s, float x, float y, float w, float h, bool right, NVGcolor color) {
    const float d = right ? 1.f : -1.f, t = w * 0.42f;  // arrow head, line thickness
    const float tip = x + (right ? w : 0), back = x + (right ? 0 : w);
    nvgBeginPath(s->vg);
    nvgMoveTo(s->vg, back, y);
    nvgLineTo(s->vg, back + d * t, y);
    nvgLineTo(s->vg, tip, y + h / 2);
    nvgLineTo(s->vg, back + d * t, y + h);
    nvgLineTo(s->vg, back, y + h);
    nvgLineTo(s->vg, tip - d * t, y + h / 2);
    nvgClosePath(s->vg);
    nvgFillColor(s->vg, color);
    nvgFill(s->vg);
}

static void optech_camera_icon(const UIState* s, float cx, float cy, float r) {
    nvgBeginPath(s->vg);
    nvgCircle(s->vg, cx, cy, r);
    nvgFillColor(s->vg, nvgRGBA(44, 44, 42, 255));
    nvgFill(s->vg);
    nvgStrokeColor(s->vg, COLOR_WHITE);
    nvgStrokeWidth(s->vg, 3);
    nvgStroke(s->vg);
    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, cx - r * 0.55f, cy - r * 0.32f, r * 1.1f, r * 0.72f, 3);
    nvgStrokeColor(s->vg, COLOR_WHITE);
    nvgStrokeWidth(s->vg, 3);
    nvgStroke(s->vg);
    nvgBeginPath(s->vg);
    nvgCircle(s->vg, cx, cy + r * 0.04f, r * 0.2f);
    nvgStroke(s->vg);
}

static void optech_dist_text(char* str, size_t n, int dist_m, const char* suffix) {
    if (dist_m >= 1000) snprintf(str, n, "%.1fkm%s", dist_m / 1000.f, suffix);
    else snprintf(str, n, "%dm%s", dist_m, suffix);
}

// big white turn arrow drawn with lines (the old turn images are mostly empty space and looked tiny, user 2026-11-23)
// kind: 1 left turn, 2 right turn, 3 left lane, 4 right lane, 5 U-turn. (x, y) top left of an S x S square.
static void optech_turn_arrow(const UIState* s, float x, float y, float S, int kind) {
    const bool mirror = kind == 2 || kind == 4;
    auto X = [&](float u) { return x + (mirror ? 1.f - u : u) * S; };
    auto Y = [&](float v) { return y + v * S; };
    NVGcontext* vg = s->vg;
    nvgSave(vg);
    nvgStrokeColor(vg, COLOR_WHITE);
    nvgFillColor(vg, COLOR_WHITE);
    nvgStrokeWidth(vg, S * 0.17f);
    nvgLineCap(vg, NVG_BUTT);
    nvgLineJoin(vg, NVG_ROUND);
    nvgBeginPath(vg);
    if (kind == 1 || kind == 2) {
        nvgMoveTo(vg, X(0.66f), Y(1.0f));
        nvgLineTo(vg, X(0.66f), Y(0.52f));
        nvgQuadTo(vg, X(0.66f), Y(0.32f), X(0.46f), Y(0.32f));
        nvgLineTo(vg, X(0.34f), Y(0.32f));
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, X(0.04f), Y(0.32f)); nvgLineTo(vg, X(0.38f), Y(0.04f)); nvgLineTo(vg, X(0.38f), Y(0.60f));
    } else if (kind == 3 || kind == 4) {
        nvgMoveTo(vg, X(0.70f), Y(1.0f));
        nvgLineTo(vg, X(0.70f), Y(0.78f));
        nvgBezierTo(vg, X(0.70f), Y(0.56f), X(0.34f), Y(0.56f), X(0.34f), Y(0.36f));
        nvgLineTo(vg, X(0.34f), Y(0.30f));
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, X(0.34f), Y(0.0f)); nvgLineTo(vg, X(0.62f), Y(0.34f)); nvgLineTo(vg, X(0.06f), Y(0.34f));
    } else {  // U-turn (to the left, as in Korea)
        nvgMoveTo(vg, X(0.74f), Y(1.0f));
        nvgLineTo(vg, X(0.74f), Y(0.42f));
        nvgArc(vg, X(0.52f), Y(0.42f), S * 0.22f, 0, NVG_PI, NVG_CCW);
        nvgLineTo(vg, X(0.30f), Y(0.62f));
        nvgStroke(vg);
        nvgBeginPath(vg);
        nvgMoveTo(vg, X(0.30f), Y(0.96f)); nvgLineTo(vg, X(0.04f), Y(0.62f)); nvgLineTo(vg, X(0.56f), Y(0.62f));
    }
    nvgClosePath(vg);
    nvgFill(vg);
    nvgRestore(vg);
}

static void drawOptechHud(const UIState* s) {
    SubMaster& sm = *(s->sm);
    const UIScene& scene = s->scene;
    const auto controls_state = sm["controlsState"].getControlsState();
    const auto car_state = sm["carState"].getCarState();
    const auto lp = sm["longitudinalPlan"].getLongitudinalPlan();
    const auto road = sm["roadLimitSpeed"].getRoadLimitSpeed();
    const auto car_params = sm["carParams"].getCarParams();
    const bool metric = scene.is_metric;
    const float unit = metric ? MS_TO_KPH : MS_TO_MPH;
    const bool blink_on = (sm.frame % 12) < 7;
    char str[128];

    const int enabled = controls_state.getEnabled();
    const int longActiveUser = controls_state.getLongActiveUser();
    const float cur_speed = std::max(0.f, car_state.getVEgoCluster() * unit);

    // ---------------- navigation values (same rules as the carrot HUD) ----------------
    const int activeNDA = road.getActive();
    const int roadLimitSpeed = road.getRoadLimitSpeed();
    const int camType = road.getCamType();
    const int xSignType = road.getXSignType();
    int limit_speed = 0, left_dist = 0;
    bool section = false;
    if (road.getCamLimitSpeed() > 0 && road.getCamLimitSpeedLeftDist() > 0) {
        limit_speed = road.getCamLimitSpeed(); left_dist = road.getCamLimitSpeedLeftDist();
    } else if (road.getSectionLimitSpeed() > 0 && road.getSectionLeftDist() > 0) {
        limit_speed = road.getSectionLimitSpeed(); left_dist = road.getSectionLeftDist(); section = true;
    } else if (road.getXSpdLimit() > 0 && road.getXSpdDist() > 0) {
        limit_speed = road.getXSpdLimit(); left_dist = road.getXSpdDist();
    }
    const bool bump = limit_speed > 0 && (xSignType == 124 || camType == 22);
    const int cam_limit = (limit_speed > 0 && !bump) ? limit_speed : 0;
    const int road_limit = (roadLimitSpeed > 0 && roadLimitSpeed < 200) ? roadLimitSpeed : 0;

    // ======================= bottom left =======================
    const float lx = 30;
    float ry = s->fb_h - 20 - 54;  // badge row top (badges are 54 high, at bottom)
    float bx = lx;
    nvgFontFace(s->vg, BOLD);
    // (no time / date in this HUD: user 2026-11-23)
    // 2) badges, only the ones that are on, packed from the left
    if (s->show_conn_info) {
        const int sccBus = (int)car_params.getSccBus();
        const int hda_limit = car_state.getSpeedLimit(), hda_dist = car_state.getSpeedLimitDistance();
        const int naviCluster = (int)car_params.getNaviCluster();
        const bool radar_tracks = s->radar_tracks;
        if (sccBus == 2) { ui_draw_image(s, {(int)bx, (int)ry, 120, 54}, "ic_scc2", 1.0f); bx += 130; }
        const char* nav = activeNDA >= 200 ? "ic_apn" : (hda_limit > 0 && hda_dist > 0) ? "ic_hda" : activeNDA >= 100 ? "ic_apm"
                        : activeNDA % 100 > 0 ? "ic_nda" : naviCluster > 0 ? "ic_hda" : nullptr;
        if (nav) { ui_draw_image(s, {(int)bx, (int)ry, 120, 54}, nav, 1.0f); bx += 130; }
        if (radar_tracks) { ui_draw_image(s, {(int)bx, (int)ry, 240, 54}, "ic_radartracks", 1.0f); bx += 250; }
    }
    g_gps_x = bx;  // ui_draw_gps_info draws the GPS bars + text right after

    // 5) following distance + gear in one box (above badges row)
    const float CW = 340, cellw = (CW - 20) / 2, sg = 124;  // column width, sign cell width, sign size (both signs)
    const float pill_h = 56;  // "550m 남음" box height
    const NVGcolor label_c = nvgRGBA(211, 209, 199, 255);
    const float gap_h = 96, gap_top = ry - 14 - gap_h;
    {
        const float w = CW, h = gap_h, top = gap_top;
        carrot_round_box(s, lx, top, w, h, nvgRGBA(0, 0, 0, 90), nvgRGBA(136, 135, 128, 255), 14, 2);
        // stock SCC: the car's own gap setting (SCC11 TauGapSet); openpilot long: openpilot's gap
        const int gap = car_params.getOpenpilotLongitudinalControl() ? (int)controls_state.getLongCruiseGap()
                                                                     : (int)car_state.getCruiseGap();
        const float gl = lx + 20, gr = lx + 214;  // left part
        optech_text_vc(s, gl, top + 30, "차간 거리", 30, label_c, NVG_ALIGN_LEFT);  // the lit steps show the setting
        const float sgap = 8, segw = (gr - gl - 3 * sgap) / 4, segh = 24, segy = top + 55;
        for (int i = 0; i < 4; i++) {
            nvgBeginPath(s->vg);
            nvgRoundedRect(s->vg, gl + i * (segw + sgap), segy, segw, segh, 6);
            nvgFillColor(s->vg, i < gap ? nvgRGBA(151, 196, 89, 255) : nvgRGBA(255, 255, 255, 45));
            nvgFill(s->vg);
        }
        // divider
        nvgBeginPath(s->vg);
        nvgMoveTo(s->vg, lx + 234, top + 16); nvgLineTo(s->vg, lx + 234, top + h - 16);
        nvgStrokeColor(s->vg, nvgRGBA(136, 135, 128, 255)); nvgStrokeWidth(s->vg, 2); nvgStroke(s->vg);
        // gear tile
        const auto gear = car_state.getGearShifter();
        NVGcolor gfill = nvgRGBA(95, 94, 90, 255);
        if (gear == cereal::CarState::GearShifter::DRIVE || gear == cereal::CarState::GearShifter::SPORT ||
            gear == cereal::CarState::GearShifter::LOW || gear == cereal::CarState::GearShifter::ECO) gfill = nvgRGBA(59, 109, 17, 255);
        else if (gear == cereal::CarState::GearShifter::REVERSE) gfill = nvgRGBA(226, 75, 74, 255);
        else if (gear == cereal::CarState::GearShifter::NEUTRAL) gfill = nvgRGBA(186, 117, 23, 255);
        const float ts = 68, tcx = (lx + 234 + lx + w) / 2, tcy = top + h / 2;
        carrot_round_box(s, tcx - ts / 2, tcy - ts / 2, ts, ts, gfill, COLOR_WHITE, 12, 3);
        optech_text_vc(s, tcx, tcy, carrot_gear_text(gear), 54, COLOR_WHITE, NVG_ALIGN_CENTER);
    }

    // 4) set speed box (anchored above Gap + Gear regardless of sign_row)
    const float applyMaxSpeed = controls_state.getVCruiseOut();
    const float cruiseMaxSpeed = controls_state.getVCruiseCluster();
    const float set_h = 124, y3 = gap_top - 14 - set_h;
    {
        const float w = CW, h = set_h;
        NVGcolor fill = nvgRGBA(95, 94, 90, 230), line = nvgRGBA(136, 135, 128, 255);
        NVGcolor txt = COLOR_WHITE;
        bool draw = true;
        if (enabled && cam_limit > 0) {
            const bool over = cur_speed > (metric ? cam_limit : cam_limit * KM_TO_MILE) + 2.0f;
            if (over) { fill = nvgRGBA(226, 75, 74, 240); line = COLOR_WHITE; draw = blink_on; }
            else { fill = nvgRGBA(59, 109, 17, 235); line = COLOR_WHITE; }
        } else if (enabled && longActiveUser > 0) {
            fill = nvgRGBA(24, 95, 165, 235); line = COLOR_WHITE;
        } else if (enabled) {
            fill = nvgRGBA(255, 255, 255, 64); line = COLOR_WHITE;
        } else {
            txt = nvgRGBA(180, 178, 169, 255);
        }
        if (draw) carrot_round_box(s, lx, y3, w, h, fill, line, 16, 3);
        optech_text(s, lx + 20, y3 + 42, "설정 속도", 32, txt, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        if (enabled && cruiseMaxSpeed > 0 && cruiseMaxSpeed < 255) {
            snprintf(str, sizeof(str), "%d", (int)(cruiseMaxSpeed * (metric ? 1.0 : KM_TO_MILE) + 0.5));
        } else {
            snprintf(str, sizeof(str), "--");
        }
        optech_text(s, lx + 20, y3 + h - 4, str, 80, txt, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        const bool slowing = enabled && longActiveUser > 0 && applyMaxSpeed > 0 && applyMaxSpeed + 0.5f < cruiseMaxSpeed;
        if (slowing) {
            optech_text(s, lx + w - 20, y3 + 42, "감속 중", 32, nvgRGBA(250, 199, 117, 255), NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
            snprintf(str, sizeof(str), "%d", (int)(applyMaxSpeed * (metric ? 1.0 : KM_TO_MILE) + 0.5));
            optech_text(s, lx + w - 20, y3 + h - 8, str, 64, nvgRGBA(250, 199, 117, 255), NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
        }
    }

    // 3) road limit + camera / bump sign (above set speed box, only with navigation data)
    const float sign_h = 50 + sg + 12 + pill_h;
    const float y2 = y3 - 18 - sign_h;
    const bool sign_row = road_limit > 0 || limit_speed > 0;
    if (sign_row) {
        if (road_limit > 0) {
            const float ccx = lx + cellw / 2, cy = y2 + 50 + sg / 2, d = sg + 8;  // as big as the camera sign + outline
            optech_text(s, ccx, y2 + 32, "도로 제한", 32, label_c, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
            nvgBeginPath(s->vg); nvgCircle(s->vg, ccx, cy, d / 2); nvgFillColor(s->vg, nvgRGBA(226, 75, 74, 255)); nvgFill(s->vg);
            nvgBeginPath(s->vg); nvgCircle(s->vg, ccx, cy, d / 2 - 14); nvgFillColor(s->vg, COLOR_WHITE); nvgFill(s->vg);
            snprintf(str, sizeof(str), "%d", metric ? road_limit : (int)(road_limit * KM_TO_MILE + 0.5));
            optech_text(s, ccx, cy + 0.36f * 64 - 6, str, 64, nvgRGBA(44, 44, 42, 255), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
        }
        if (limit_speed > 0) {
            const float cellx = lx + cellw + 20, ccx = cellx + cellw / 2, top = y2 + 50, sx = ccx - sg / 2;
            optech_text(s, ccx, y2 + 32, bump ? "방지턱" : section ? "구간 단속" : "단속 속도", 32, COLOR_WHITE, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
            if (bump) {
                ui_draw_image(s, {(int)sx, (int)top, (int)sg, (int)sg}, "ic_speed_bump", 1.0f);
            } else {
                carrot_round_box(s, sx, top, sg, sg, nvgRGBA(250, 199, 117, 255), nvgRGBA(44, 44, 42, 255), 14, 5);
                nvgBeginPath(s->vg); nvgRoundedRect(s->vg, sx - 4, top - 4, sg + 8, sg + 8, 17);
                nvgStrokeColor(s->vg, COLOR_WHITE); nvgStrokeWidth(s->vg, 3); nvgStroke(s->vg);
                snprintf(str, sizeof(str), "%d", metric ? limit_speed : (int)(limit_speed * KM_TO_MILE + 0.5));
                optech_text(s, ccx, top + sg / 2 + 0.36f * 68 - 6, str, 68, nvgRGBA(44, 44, 42, 255), NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
                optech_camera_icon(s, sx + sg - 20, top + 20, 15);  // small, inside the sign's top right corner
            }
            if (left_dist > 0) {
                optech_dist_text(str, sizeof(str), left_dist, " 남음");
                // whole cell wide (its right edge lines up with the set speed box), text as big as fits
                const float bw = cellw, bh = pill_h, btop = top + sg + 12;
                const float a = std::clamp((600.f - left_dist) / 600.f, 0.25f, 1.f);  // fills redder as it gets close
                carrot_round_box(s, ccx - bw / 2, btop, bw, bh, nvgRGBA(226, 75, 74, (int)(255 * a)), COLOR_WHITE, 12, 3);
                float fs = 36, tb[4];
                nvgFontFace(s->vg, BOLD);
                nvgFontSize(s->vg, fs);
                nvgTextBounds(s->vg, 0, 0, str, NULL, tb);
                const float maxw = bw - 16;
                if (tb[2] - tb[0] > maxw) fs *= maxw / (tb[2] - tb[0]);
                optech_text(s, ccx, btop + bh / 2 - fs * 0.2f, str, fs, COLOR_WHITE, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            }
        }
    }

    // ======================= bottom centre =======================
    const float cx = s->fb_w / 2.f;
    float speed_half_w = 96.f;
    float speed_right_edge = cx + 96.f;
    {
        // big speed anchored near bottom edge; GPS speed + unit stacked neatly to its right so the centre stays low
        const float a = car_state.getAEgo();
        NVGcolor c = COLOR_WHITE;
        if (car_state.getBrakePressed() || a < -0.3f) c = nvgRGBA(240, 149, 149, 255);
        else if (a > 0.3f) c = nvgRGBA(151, 196, 89, 255);
        snprintf(str, sizeof(str), "%.0f", cur_speed);
        const float sp_fs = 168.f;
        nvgFontFace(s->vg, BOLD);
        nvgFontSize(s->vg, sp_fs);
        float sb[4];
        nvgTextBounds(s->vg, 0, 0, str, NULL, sb);
        speed_half_w = (cur_speed >= 99.5f) ? std::max(138.f, (sb[2] - sb[0]) * 0.5f)
                                            : std::max(96.f, (sb[2] - sb[0]) * 0.5f);
        optech_text(s, cx, s->fb_h - 18, str, sp_fs, c, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);

        const float sx = cx + speed_half_w + 18.f;
        const char* unit_str = metric ? "km/h" : "mph";
        const bool gps_ok = sm.alive("gpsLocationExternal") && (sm["gpsLocationExternal"].getGpsLocationExternal().getFlags() & 1);
        if (gps_ok) snprintf(str, sizeof(str), "GPS %d", (int)(std::max(sm["gpsLocationExternal"].getGpsLocationExternal().getSpeed() * unit, 0.f) + 0.5f));
        else snprintf(str, sizeof(str), "GPS --");
        nvgFontFace(s->vg, BOLD);
        nvgFontSize(s->vg, 36.f);
        float gb[4], ub[4];
        nvgTextBounds(s->vg, 0, 0, str, NULL, gb);
        nvgFontSize(s->vg, 34.f);
        nvgTextBounds(s->vg, 0, 0, unit_str, NULL, ub);
        speed_right_edge = sx + std::max(gb[2] - gb[0], ub[2] - ub[0]);
        optech_text(s, sx, s->fb_h - 64, str, 36, gps_ok ? nvgRGBA(151, 196, 89, 255) : nvgRGBA(136, 135, 128, 255), NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        optech_text(s, sx, s->fb_h - 22, unit_str, 34, nvgRGBA(211, 209, 199, 255), NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
    }
    {
        // turn signal arrows flanking the speed cluster along the bottom bar
        const bool lb = car_state.getLeftBlinker(), rb = car_state.getRightBlinker();
        if (lb || rb) {
            const int step = (sm.frame / 4) % 4;  // 0..3
            const float w = 44, h = 82, gap = 32, y = s->fb_h - 110;
            const float lx_inner = cx - speed_half_w - 22.f;
            const float rx_inner = speed_right_edge + 20.f;
            for (int i = 0; i < 3; i++) {
                const int a = (step > i) ? 255 : 70;
                const NVGcolor col = nvgRGBA(239, 159, 39, a);
                if (lb) optech_chevron(s, lx_inner - i * gap - w, y, w, h, false, col);
                if (rb) optech_chevron(s, rx_inner + i * gap, y, w, h, true, col);
            }
        }
    }
    // traffic light (left of speed) + navigation turn guidance (right of speed), keeping the centre lane open.
    // Green (tl_mode == 2) is suppressed while cruising so TrafficStopMode > 0 does not keep it lit all drive.
    const int tl_ts = lp.getTrafficState() % 1000;
    const int tl_mode = tl_ts >= 100 ? 3 : (tl_ts % 100);
    const auto x_state = lp.getXState();
    const bool at_stop = car_state.getVEgo() < 3.0f || car_state.getStandstill() ||
                         x_state == cereal::LongitudinalPlan::XState::E2E_STOP ||
                         x_state == cereal::LongitudinalPlan::XState::SOFT_HOLD ||
                         x_state == cereal::LongitudinalPlan::XState::E2E_CRUISE_PREPARE;
    const bool has_tl = (tl_mode == 1 || tl_mode == 3) || (tl_mode == 2 && at_stop);

    const int turn = road.getXTurnInfo();
    int turn_arrow = 0;
    const char* turn_label = nullptr;
    if (turn > 0 && activeNDA > 0) {
        switch (turn) {
        case 1: turn_arrow = 1; turn_label = "좌회전"; break;
        case 2: turn_arrow = 2; turn_label = "우회전"; break;
        case 3: turn_arrow = 3; turn_label = "왼쪽 차로"; break;
        case 4: turn_arrow = 4; turn_label = "오른쪽 차로"; break;
        case 5: turn_arrow = 5; turn_label = "U턴"; break;
        case 35: turn_label = "좌측 고가 진입"; break;
        case 43: turn_label = "지하차도 우측"; break;
        case 48: turn_label = "휴게소"; break;
        default: break;
        }
    }
    const bool has_turn = turn_label != nullptr;
    {
        if (has_tl) {
            const float tl_w = 204, tl_h = 64, tl_r = 21;
            const float tl_x = cx - speed_half_w - 36.f - tl_w;
            const float tl_y = s->fb_h - 184.f;
            carrot_round_box(s, tl_x, tl_y, tl_w, tl_h, nvgRGBA(44, 44, 42, 240), COLOR_WHITE, 14, 3);
            const NVGcolor off = nvgRGBA(95, 94, 90, 255);
            const NVGcolor on[3] = {nvgRGBA(226, 75, 74, 255), nvgRGBA(239, 159, 39, 255), nvgRGBA(99, 153, 34, 255)};
            const int lit = tl_mode == 1 ? 0 : tl_mode == 3 ? 1 : 2;  // 1 red, 3 yellow, 2 green
            for (int i = 0; i < 3; i++) {
                nvgBeginPath(s->vg);
                nvgCircle(s->vg, tl_x + tl_w / 2 + (i - 1) * 62, tl_y + tl_h / 2, tl_r);
                nvgFillColor(s->vg, i == lit ? on[i] : off);
                nvgFill(s->vg);
            }
        }
        if (has_turn) {
            const float turn_h = 66, S = 52;
            const float turn_x = cx + speed_half_w + 18.f;
            const float turn_y = s->fb_h - 186.f;
            char turn_str[128] = "";
            const int dist = road.getXDistToTurn();
            if (dist > 0) { char d[32]; optech_dist_text(d, sizeof(d), dist, ""); snprintf(turn_str, sizeof(turn_str), "%s %s", turn_label, d); }
            else snprintf(turn_str, sizeof(turn_str), "%s", turn_label);
            float t_fs = 40.f;
            nvgFontFace(s->vg, BOLD);
            nvgFontSize(s->vg, t_fs);
            float b[4]; nvgTextBounds(s->vg, 0, 0, turn_str, NULL, b);
            const float turn_iw = turn_arrow ? S + 14.f : 0.f;
            const float max_turn_w = (s->show_record_btn ? (s->fb_w - 360.f) : (s->fb_w - 205.f)) - turn_x;
            float turn_w = (b[2] - b[0]) + turn_iw + 38.f;
            if (max_turn_w > 140.f && turn_w > max_turn_w) {
                const float avail = std::max(60.f, max_turn_w - turn_iw - 38.f);
                if (b[2] - b[0] > avail) {
                    t_fs *= avail / (b[2] - b[0]);
                    nvgFontSize(s->vg, t_fs);
                    nvgTextBounds(s->vg, 0, 0, turn_str, NULL, b);
                }
                turn_w = (b[2] - b[0]) + turn_iw + 38.f;
            }
            carrot_round_box(s, turn_x, turn_y, turn_w, turn_h, nvgRGBA(0, 0, 0, 145), nvgRGBA(255, 255, 255, 65), 12, 1.5f);
            if (turn_arrow) optech_turn_arrow(s, turn_x + 18, turn_y + (turn_h - S) / 2, S, turn_arrow);
            optech_text(s, turn_x + 20 + turn_iw, turn_y + turn_h / 2 + 2, turn_str, t_fs, COLOR_WHITE, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        }
    }

    // ======================= lead info under the lead =======================
    {
        const auto lead = sm["radarState"].getRadarState().getLeadOne();
        if (lead.getStatus()) {
            const float d = lead.getDRel(), vl = std::max(0.f, lead.getVLead() * unit), ve = car_state.getVEgo();
            const bool gap_ok = ve > 1.0f;
            const float tgap = gap_ok ? d / ve : 0.f;
            const float chev_h = 30.f, chev_gap = 6.f, lh = 50.f, fs = 34.f;
            const float bottom_hud_top = (has_tl || has_turn) ? (s->fb_h - 186.f) : (s->fb_h - 148.f);
            const float max_lead_y = std::max(280.f, bottom_hud_top - 10.f - chev_h - chev_gap - lh);

            char s_dist[24], s_spd[24], s_gap[24];
            snprintf(s_dist, sizeof(s_dist), "%.0fm", d);
            snprintf(s_spd, sizeof(s_spd), "%.0f%s", vl, metric ? "km/h" : "mph");
            if (gap_ok) snprintf(s_gap, sizeof(s_gap), "%.1f%s", tgap, "초");

            nvgFontFace(s->vg, BOLD);
            nvgFontSize(s->vg, fs);
            float b1[4], b2[4], b3[4] = {0, 0, 0, 0};
            nvgTextBounds(s->vg, 0, 0, s_dist, NULL, b1);
            nvgTextBounds(s->vg, 0, 0, s_spd, NULL, b2);
            if (gap_ok) nvgTextBounds(s->vg, 0, 0, s_gap, NULL, b3);
            const float w1 = b1[2] - b1[0], w2 = b2[2] - b2[0], w3 = gap_ok ? (b3[2] - b3[0]) : 0.f;
            const float sep_w = 26.f, pad_x = 20.f;
            const float pill_w = pad_x * 2.f + w1 + sep_w + w2 + (gap_ok ? (sep_w + w3) : 0.f);

            const QPointF& v = scene.lead_vertices[0];
            const float min_x = 30.f + CW + 20.f + pill_w * 0.5f;
            const float max_x = s->fb_w - 30.f - 160.f - 20.f - pill_w * 0.5f;
            const float x = (min_x < max_x) ? std::clamp((float)v.x(), min_x, max_x) : cx;
            const float y = std::clamp((float)v.y(), 280.f, max_lead_y);

            // chevron anchored right at the rear bumper so the lead car body above stays uncovered
            const NVGcolor chev_col = (gap_ok && tgap < 1.0f) ? nvgRGBA(226, 75, 74, 255) : nvgRGBA(239, 159, 39, 255);
            nvgBeginPath(s->vg);
            nvgMoveTo(s->vg, x, y);
            nvgLineTo(s->vg, x + 24.f, y + chev_h);
            nvgLineTo(s->vg, x - 24.f, y + chev_h);
            nvgClosePath(s->vg);
            nvgFillColor(s->vg, chev_col);
            nvgFill(s->vg);
            nvgStrokeColor(s->vg, nvgRGBA(0, 0, 0, 160));
            nvgStrokeWidth(s->vg, 2.f);
            nvgStroke(s->vg);

            const float top = y + chev_h + chev_gap, cy = top + lh * 0.5f;
            const float px = x - pill_w * 0.5f;
            carrot_round_box(s, px, top, pill_w, lh, nvgRGBA(18, 18, 18, 175), nvgRGBA(255, 255, 255, 75), 14, 1.5f);

            float tx = px + pad_x;
            const NVGcolor dot_col = nvgRGBA(180, 178, 169, 200);
            optech_text_vc(s, tx, cy, s_dist, fs, COLOR_WHITE, NVG_ALIGN_LEFT);
            tx += w1;
            nvgBeginPath(s->vg); nvgCircle(s->vg, tx + sep_w * 0.5f, cy, 3.5f); nvgFillColor(s->vg, dot_col); nvgFill(s->vg);
            tx += sep_w;
            optech_text_vc(s, tx, cy, s_spd, fs, COLOR_WHITE, NVG_ALIGN_LEFT);
            if (gap_ok) {
                tx += w2;
                nvgBeginPath(s->vg); nvgCircle(s->vg, tx + sep_w * 0.5f, cy, 3.5f); nvgFillColor(s->vg, dot_col); nvgFill(s->vg);
                tx += sep_w;
                const NVGcolor gc = tgap < 1.0f ? nvgRGBA(240, 149, 149, 255) : tgap < 2.0f ? nvgRGBA(250, 199, 117, 255) : COLOR_WHITE;
                optech_text_vc(s, tx, cy, s_gap, fs, gc, NVG_ALIGN_LEFT);
            }
        }
    }

    // ======================= bottom-right column =======================
    {
        // three boxes of the same size stacked up from the bottom right: wheel, CPU, tyres
        const QRect wr = optech_wheel_rect(s->fb_w, s->fb_h);
        const float colw = wr.width(), boxh = wr.height(), gap = 14, rx = wr.x(), rcx = rx + colw / 2;
        const NVGcolor box_fill = nvgRGBA(0, 0, 0, 100), box_line = nvgRGBA(180, 178, 169, 255), lab = nvgRGBA(211, 209, 199, 255);
        float top = wr.y();
        // steering wheel like OPKR: the comma wheel picture on a circle in the status colour, turning with the wheel.
        // Tap: path offset panel
        carrot_round_box(s, rx, top, colw, boxh, box_fill, box_line, 14, 3);
        const QColor& sc = bg_colors[s->status];
        const NVGcolor wc = enabled ? nvgRGBA(sc.red(), sc.green(), sc.blue(), sc.alpha()) : nvgRGBA(0x17, 0x33, 0x49, 0xc8);
        ui_draw_circle_image_rotation(s, rcx, top + boxh / 2, 66, "wheel", wc, 1.0f, car_state.getSteeringAngleDeg());

        // CPU temperature / CPU use (화면 탭 '기기 상태'; the tyre box moves down when it is hidden)
        if (s->show_device_stat) {
        auto ds = sm["deviceState"].getDeviceState();
        float temp = 0.f; int use = 0;
        const auto tc = ds.getCpuTempC(); const auto cu = ds.getCpuUsagePercent();
        for (int i = 0; i < (int)std::size(tc); i++) temp += tc[i];
        if (std::size(tc) > 0) temp /= std::size(tc);
        for (int i = 0; i < (int)std::size(cu); i++) use += cu[i];
        if (std::size(cu) > 0) use /= (int)std::size(cu);
        top -= boxh + gap;
        carrot_round_box(s, rx, top, colw, boxh, box_fill, box_line, 14, 3);
        optech_text_vc(s, rcx, top + 26, "CPU 온도", 30, lab, NVG_ALIGN_CENTER);
        snprintf(str, sizeof(str), "%.0f%s", temp, "°C");
        optech_text_vc(s, rcx, top + 60, str, 44, temp > 80 && blink_on ? nvgRGBA(240, 149, 149, 255) : COLOR_WHITE, NVG_ALIGN_CENTER);
        optech_text_vc(s, rcx, top + 99, "CPU 사용", 30, lab, NVG_ALIGN_CENTER);
        snprintf(str, sizeof(str), "%d%%", use);
        optech_text_vc(s, rcx, top + 133, str, 44, use > 90 && blink_on ? nvgRGBA(240, 149, 149, 255) : COLOR_WHITE, NVG_ALIGN_CENTER);
        }

        // tyre pressures
        if (s->show_tpms) {
            auto tpms = car_state.getTpms();
            const float t[4] = {tpms.getFl(), tpms.getFr(), tpms.getRl(), tpms.getRr()};
            top -= boxh + gap;
            carrot_round_box(s, rx, top, colw, boxh, box_fill, box_line, 14, 3);
            optech_text_vc(s, rcx, top + 26, "공기압 psi", 30, lab, NVG_ALIGN_CENTER);
            for (int i = 0; i < 4; i++) {
                const float px = rcx + (i % 2 == 0 ? -38 : 38), py = top + (i < 2 ? 77 : 127);
                std::string v = get_tpms_text(t[i]);
                optech_text_vc(s, px, py, v.c_str(), 44, get_tpms_color(t[i]), NVG_ALIGN_CENTER);
            }
        }

        // path offset panel (opened by tapping the wheel): − / + move the path left / right, 5 cm a tap
        if (s->optech_tune_open) {
            const QRect pr = optech_tune_panel_rect(s->fb_w, s->fb_h);
            const QRect mr = optech_tune_minus_rect(s->fb_w, s->fb_h), plr = optech_tune_plus_rect(s->fb_w, s->fb_h);
            carrot_round_box(s, pr.x(), pr.y(), pr.width(), pr.height(), nvgRGBA(20, 20, 20, 230), COLOR_WHITE, 16, 3);
            optech_text(s, pr.x() + pr.width() / 2, pr.y() + 56, "경로 좌우 보정", 40, COLOR_WHITE, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
            snprintf(str, sizeof(str), "%+d cm", s->path_offset);
            optech_text(s, pr.x() + pr.width() / 2, mr.y() + mr.height() / 2 + 4, str, 56, nvgRGBA(250, 199, 117, 255), NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            carrot_round_box(s, mr.x(), mr.y(), mr.width(), mr.height(), nvgRGBA(95, 94, 90, 255), COLOR_WHITE, 14, 3);
            carrot_round_box(s, plr.x(), plr.y(), plr.width(), plr.height(), nvgRGBA(95, 94, 90, 255), COLOR_WHITE, 14, 3);
            optech_text(s, mr.x() + mr.width() / 2, mr.y() + mr.height() / 2 + 2, "-", 80, COLOR_WHITE, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            optech_text(s, plr.x() + plr.width() / 2, plr.y() + plr.height() / 2 + 2, "+", 70, COLOR_WHITE, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            optech_text(s, pr.x() + pr.width() / 2, pr.y() + pr.height() - 18, "- 왼쪽으로   + 오른쪽으로", 28, nvgRGBA(211, 209, 199, 255), NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        }
    }
}


void DrawApilot::drawLeadApilot(const UIState* s) {
    SubMaster& sm = *(s->sm);
    const bool carrot_hud = s->show_mode == 6;
    const cereal::ModelDataV2::Reader& model = sm["modelV2"].getModelV2();
    const UIScene& scene = s->scene;
    auto leads = model.getLeadsV3();
#ifndef __TEST
    //const cereal::ModelDataV2::LeadDataV3::Reader& lead_data = leads[0];
#endif
    //const QPointF& vd = s->scene.lead_vertices[0];
    //bool is_radar = s->scene.lead_radar[0];
    bool no_radar = leads[0].getProb() < .5;
    bool    uiDrawSteeringRotate = s->show_steer_rotate;

#ifndef __TEST
    if (!sm.alive("controlsState") || !sm.alive("radarState") || !sm.alive("carControl")) return;
    auto lead_radar = sm["radarState"].getRadarState().getLeadOne();
    auto lead_one = sm["modelV2"].getModelV2().getLeadsV3()[0];
#endif
#ifdef __TEST
    float radar_dist = 0;
    bool radar_detected = false;
    float vision_dist = 0.0;
#else
    bool radar_detected = lead_radar.getStatus() && lead_radar.getRadar();
    float radar_dist = radar_detected ? lead_radar.getDRel() : 0;
    float vision_dist = lead_one.getProb() > .5 ? (lead_one.getX()[0] - 0) : 0;
#endif
    auto controls_state = sm["controlsState"].getControlsState();
    auto car_control = sm["carControl"].getCarControl();
    auto car_state = sm["carState"].getCarState();
    int longActiveUser = controls_state.getLongActiveUser();
    int longActiveUserReady = controls_state.getLongActiveUserReady();

    nvgFontFace(s->vg, BOLD);
    nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);

    ui_draw_plot(s);
    ui_draw_radar_info(s);
    if (s->show_mode == 7) {  // optech 주행 화면
        drawOptechHud(s);
        return;
    }

    if (s->show_mode == 2) {
        NVGpaint track_bg;
        track_bg = nvgLinearGradient(s->vg, s->fb_w, s->fb_h - 50, s->fb_w, s->fb_h - 350,
            nvgRGBA(0, 0, 0, 250), nvgRGBA(0, 0, 0, 0));
        float x[4], y[4];
        x[0] = 0.0;
        y[0] = s->fb_h;
        x[1] = 0.0;
        y[1] = s->fb_h - 500;
        x[2] = s->fb_w;
        y[2] = s->fb_h - 500;
        x[3] = s->fb_w;
        y[3] = s->fb_h;
        ui_draw_line2(s, x, y, 4, nullptr, &track_bg, 0.0);
    }
    else if (s->show_mode == 3) {
        NVGpaint track_bg;
        track_bg = nvgLinearGradient(s->vg, 0, 0, 0, 150,
            nvgRGBA(0, 0, 0, 250), nvgRGBA(0, 0, 0, 0));
        float x[4], y[4];
        x[0] = 0.0;
        y[0] = 0;
        x[1] = 0.0;
        y[1] = 500;
        x[2] = s->fb_w;
        y[2] = 500;
        x[3] = s->fb_w;
        y[3] = 0;
        ui_draw_line2(s, x, y, 4, nullptr, &track_bg, 0.0);
    }


    static float path_fx = s->fb_w / 2;
    static float path_fy = s->fb_h - 400;
    static float path_fwidth = 160;
    static float path_bx = s->fb_w / 2;
    float alpha = 0.85;

    int len = scene.track_vertices.length();
    //int hlen = len / 2;
    float _path_x = path_fx;
    float _path_y = path_fy;
    float _path_width = path_fwidth;
    
    float lex = scene.path_end_left_vertices[0].x();
    float rex = scene.path_end_right_vertices[0].x();
    float ley = scene.path_end_left_vertices[0].y();
    float rey = scene.path_end_right_vertices[0].y();
    _path_width = rex - lex;
    _path_x = (lex + rex) / 2.;
    _path_y = (ley + rey) / 2.;
    
    if (len >= 2) {
        float lbx = scene.track_vertices[0].x();
        //float lby = scene.track_vertices[0].y();
        float rbx = scene.track_vertices[len - 1].x();
        //float rby = scene.track_vertices[len - 1].y();
        //float lex = scene.track_vertices[hlen - 1].x();
        //float ley = scene.track_vertices[hlen - 1].y();
        //float rex = scene.track_vertices[hlen].x();
        //float rey = scene.track_vertices[hlen].y();
        //_path_width = rex - lex;
        //_path_x = (lex + rex) / 2.;
        //_path_y = (ley + rey) / 2.;
        path_bx = path_bx * alpha + (lbx + rbx) / 2. * (1. - alpha);
    }
    //if (radar_detected) {
    //    _path_x = (int)std::clamp((float)vd.x(), 550.f, s->fb_w - 550.f);
    //    _path_y = (int)std::clamp((float)vd.y(), 200.f, s->fb_h - 100.f);
    //}
    _path_x = (int)std::clamp(_path_x, 550.f, s->fb_w - 550.f);
    _path_y = (int)std::clamp(_path_y, 200.f, s->fb_h - 100.f);
    if (isnan(_path_x) || isnan(_path_y) || isnan(_path_width));
    else {
        path_fx = path_fx * alpha + _path_x * (1. - alpha);
        path_fy = path_fy * alpha + _path_y * (1. - alpha);

        if (_path_width < 120.) _path_width = 120.;
        else if (_path_width > 800.) _path_width = 800.;
        path_fwidth = path_fwidth * alpha + _path_width * (1. - alpha);
    }

    //printf("%.0f, %.0f, %.0f, %.0f, %.0f\n", _path_x, _path_y, path_fx, path_fy, path_fwidth);

    int path_x = (int)path_fx;
    int path_y = (int)path_fy;
    int path_width = (int)path_fwidth;

    //if(s->show_path_end>0) ui_fill_rect(s->vg, { path_x - path_width / 2, path_y, path_width, -4 }, COLOR_RED, 5);

    // 과녁을 표시할 위치를 계산
    int icon_size = 256;
#ifdef __TEST
    //const int d_rel = 0;
    static int test_seq = 0;
    if (++test_seq > 100) test_seq = 0;
#else
    //const int d_rel = lead_data.getX()[0];
#endif
    int x = path_x;
    int y = path_y;
    y = path_y - 135;
    if (s->show_mode == 1 || s->show_mode == 2) {
        if (y > s->fb_h - 400) y = s->fb_h - 400;
    }

    if (s->show_mode == 2) {
        y = s->fb_h - 400;
        x = path_bx;
    }

    if(s->show_mode==1) 
        x = std::clamp((float)x, 550.f, s->fb_w - 550.f);

    filter_x = x;
    filter_y = y;

    // 신호등(traffic)그리기.
    // 신호등내부에는 레이더거리, 비젼거리, 정지거리, 신호대기 표시함.
    int circle_size = 160;
    NVGcolor bgColor = nvgRGBA(0, 0, 0, 166);
    const auto lp = sm["longitudinalPlan"].getLongitudinalPlan();
    float stop_dist = 0;
    bool stopping = false;
    auto hud_control = car_control.getHudControl();
#ifdef __TEST
    float radar_rel_speed = 20.0;
    if (test_seq > 50) radar_rel_speed = -20.0;
#else
    float radar_rel_speed = lead_radar.getVRel();
#endif
    float disp_dist = (radar_detected) ? radar_dist : vision_dist;
    int brake_hold = car_state.getBrakeHoldActive();
    int soft_hold = (hud_control.getSoftHold()) ? 1 : 0;

    float v_ego = sm["carState"].getCarState().getVEgoCluster();
    float v_ego_kph = v_ego * MS_TO_KPH;
    float cur_speed = v_ego * (s->scene.is_metric ? MS_TO_KPH : MS_TO_MPH);
    if (cur_speed < 0.0) cur_speed = 0.0;
    bool brake_valid = car_state.getBrakeLights();
    bool bsd_l = car_state.getLeftBlindspot();
    bool bsd_r = car_state.getRightBlindspot();
    // 차로변경/자동턴 표시
    bool showDistInfo = true;
    bool showBg = (disp_dist > 0.0) ? true : false;
    auto lateralPlan = sm["lateralPlan"].getLateralPlan();
    auto desire = lateralPlan.getDesire();
    auto laneChangeDirection = lateralPlan.getLaneChangeDirection();
    auto desireEvent = lateralPlan.getDesireEvent();
    float desireStateTurnLeft = (desire == cereal::LateralPlan::Desire::TURN_LEFT) ? 1 : 0;
    float desireStateTurnRight = (desire == cereal::LateralPlan::Desire::TURN_RIGHT) ? 1 : 0;
    float desireStateLaneChangeLeft = (desire == cereal::LateralPlan::Desire::LANE_CHANGE_LEFT) ? 1 : 0;
    float desireStateLaneChangeRight = (desire == cereal::LateralPlan::Desire::LANE_CHANGE_RIGHT) ? 1 : 0;

    if (desire == cereal::LateralPlan::Desire::NONE) {
        auto meta = sm["modelV2"].getModelV2().getMeta();
        desireStateTurnLeft = meta.getDesireState()[1];
        desireStateTurnRight = meta.getDesireState()[2];
        desireStateLaneChangeLeft = meta.getDesireState()[3];
        desireStateLaneChangeRight = meta.getDesireState()[4];
    }
    bool leftBlinker = car_state.getLeftBlinker();
    bool rightBlinker = car_state.getRightBlinker();
    static int blinkerTimer = 0;
    blinkerTimer = (blinkerTimer + 1) % 16;
    bool blinkerOn = (blinkerTimer <= 16 / 2);
    int trafficState = lp.getTrafficState();
    char    str[128];

#ifdef __TEST
    static int _desire = 0.;
    desireStateTurnLeft = 0.0;
    desireStateTurnRight = 0.0;
    desireStateLaneChangeLeft = 0.0;
    desireStateLaneChangeRight = 0.0;

    if (_desire++ > 200) _desire = 0;
    if (_desire > 100) brake_valid = true; else brake_valid = false;
    if (_desire < 50) {
        desireStateTurnLeft = 1.0; 
        trafficState = 0;
    }
    else if (_desire < 100) {
        desireStateTurnRight = 1.0;
        trafficState = 1;
    }
    else if (_desire < 150) {
        desireStateLaneChangeLeft = 1.0;
        trafficState = 2;
    }
    else {
        desireStateLaneChangeRight = 1.0;
        trafficState = 3;
    }
#endif
    if ((desireStateTurnLeft > 0.5) || (desireStateTurnRight > 0.5) || (desireStateLaneChangeLeft > 0.5) || (desireStateLaneChangeRight > 0.5)) {
        showBg = true;
        showDistInfo = false;
    }
    int trafficMode = 0;
    if (true) {  // 핸들표시가 없는경우만 표시, 핸들표시모드인경우, 핸들아이콘으로 표시됨.
#ifdef __TEST
        if (s->show_steer_mode == 2) ui_draw_image(s, { x - icon_size / 2 - 30, y + icon_size / 2 - 60, icon_size, icon_size }, "ic_traffic_green", 1.0f);
        if (s->show_steer_mode == 2) ui_draw_image(s, { x - icon_size / 2 + 30, y + icon_size / 2 - 60, icon_size, icon_size }, "ic_traffic_red", 1.0f);
#endif
        if (longActiveUser <= 0) trafficMode = 0;  // 크루즈가 꺼져있으면... 신호등을 모두 꺼버려?
        else if (trafficState >= 100) trafficMode = 3; // yellow
        else {
            switch (lp.getTrafficState() % 100) {
            case 0: trafficMode = 0; break;
            case 1: trafficMode = 1;    // red
                stop_dist = lp.getXStop();
                stopping = true;
                if (s->show_mode == 4 || s->show_mode == 5) {
                    //if (s->show_path_end == 1 && s->show_steer_mode == 2) ui_draw_image(s, { x - icon_size / 2 + 30, y + icon_size / 2 - 60, icon_size, icon_size }, "ic_traffic_red", 1.0f);
                }
                else if (s->show_steer_mode == 2) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_traffic_red", 1.0f);
                showBg = true;
                break;
            case 2: trafficMode = 2;
                if (s->show_mode == 4 || s->show_mode == 5) {
                    //if (s->show_path_end == 1 && s->show_steer_mode == 2) ui_draw_image(s, { x - icon_size / 2 - 30, y + icon_size / 2 - 60, icon_size, icon_size }, "ic_traffic_green", 1.0f);
                }
                else if (s->show_steer_mode == 2) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_traffic_green", 1.0f);
                break; // green // 표시안함.
            case 3: trafficMode = 3; showBg = true; break; // yellow
            }
        }
#ifdef __TEST
        static int traffic = 0;
        if (traffic++ > 200) traffic = 0;
        if (traffic < 50) trafficMode = 0;
        else if (traffic < 100) trafficMode = 1;
        else if (traffic < 150) trafficMode = 2;
        else trafficMode = 3;

        if (traffic < 100) { leftBlinker = true; }
        else { rightBlinker = true; }
#endif
        // steer handle 그리기..
        float steer_angle = car_state.getSteeringAngleDeg();
#ifdef __TEST
        static float steer_ang = 0.0;
        steer_ang += 1.0;
        steer_angle = steer_ang;
#endif
        if (s->show_steer_mode == 1) {
            if (uiDrawSteeringRotate) {      // 시간이 많이(3msec)걸려 3번에 한번씩만 그리자..
                switch (trafficMode) {
                case 0: ui_draw_circle_image_rotation(s, x, y, icon_size / 2., "ic_steer_momo", nvgRGBA(0, 0, 0, 0), 0.7f, steer_angle); break;
                case 1: ui_draw_circle_image_rotation(s, x, y, icon_size / 2., "ic_steer_red", nvgRGBA(0, 0, 0, 0), 0.7f, steer_angle); break;
                case 2: ui_draw_circle_image_rotation(s, x, y, icon_size / 2., "ic_steer_green", nvgRGBA(0, 0, 0, 0), 0.7f, steer_angle); break;
                case 3: ui_draw_circle_image_rotation(s, x, y, icon_size / 2., "ic_steer_yellow", nvgRGBA(0, 0, 0, 0), 0.7f, steer_angle); break;
                }
            }
            else ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_steer_momo", 0.7f);
            bgColor = nvgRGBA(0, 0, 0, 0);
        }
        else if (s->show_steer_mode == 0) {            
            if (uiDrawSteeringRotate) {      
                ui_draw_circle_image_rotation(s, x, y, icon_size / 2., "ic_steer_momo", nvgRGBA(0, 0, 0, 0), 0.7f, steer_angle);
            }
            else ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_steer_momo", 0.7f);
            switch (trafficMode) {
            case 0: bgColor = nvgRGBA(0, 0, 0, 0); break;
            case 1: bgColor = nvgRGBA(255, 0, 0, 100); break;
            case 2: bgColor = nvgRGBA(0, 255, 0, 100); break;
            case 3: bgColor = nvgRGBA(255, 255, 0, 100); break;
            }
        }
        else {
            showBg = false;
        }
        if (showBg) {
            nvgBeginPath(s->vg);
            nvgCircle(s->vg, x, y, circle_size/2.);
            nvgFillColor(s->vg, bgColor);
            nvgFill(s->vg);

        }


        // 차로변경, 턴 표시~
        if (true) {            
            if (desireStateTurnLeft > 0.5) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_turn_l", 1.0f);
            else if (desireStateTurnRight > 0.5) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_turn_r", 1.0f);
            else if (desireStateLaneChangeLeft > 0.5) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_l", 1.0f);
            else if (desireStateLaneChangeRight > 0.5) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_r", 1.0f);
            if (desireEvent == 57) {
                ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_steer", 1.0f);
                ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_l", 1.0f);
            }
            else if (desireEvent == 58) {
                ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_steer", 1.0f);
                ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_r", 1.0f);
            }
            else if (desireEvent == 71) {
                if (laneChangeDirection == cereal::LateralPlan::LaneChangeDirection::LEFT) {
                    ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_inhibit", 1.0f);
                    ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_l", 1.0f);
                }
                else if (laneChangeDirection == cereal::LateralPlan::LaneChangeDirection::RIGHT) {
                    ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_inhibit", 1.0f);
                    ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_lane_change_r", 1.0f);
                }
            }

        }
        // blinker 표시~~
        if (true) {
            if (rightBlinker && blinkerOn) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_blinker_r", 1.0f);
            if (leftBlinker && blinkerOn) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_blinker_l", 1.0f);
        }
        // BSD 표시
        if (false) {
            if (bsd_l) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_bsd_l", 1.0f);
            if (bsd_r) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, "ic_bsd_r", 1.0f);
        }

#ifdef __TEST
        radar_detected = true;
        disp_dist = 127.0;
        stop_dist = 112.0;
        stopping = true;
#endif

        NVGcolor textColor = COLOR_WHITE;
        NVGcolor borderColor = COLOR_BLACK;

        if (radar_detected) {
            sprintf(str, "%.1f %s", (cur_speed + radar_rel_speed * 3.6) * (s->scene.is_metric?1.0:KM_TO_MILE), s->scene.is_metric?"km/h" : "mile");
            if (radar_rel_speed < -0.1) {
                textColor = COLOR_RED; 
                borderColor = COLOR_BLACK;
            }
            else {
                textColor = COLOR_GREEN;
                borderColor = COLOR_BLACK;
            }

            nvgFontSize(s->vg, 40);
        }
        if (s->show_mode >= 2) {
            if(!no_radar && s->show_path_end != 1) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, (radar_detected) ? "ic_radar" : "ic_radar_vision", 1.0f);
        }
        else ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2, icon_size, icon_size }, (no_radar) ? "ic_radar_no" : (radar_detected) ? "ic_radar" : "ic_radar_vision", 1.0f);

        float disp_size = (s->show_path_end==1)?60.0:45.0;
        //int dist_y = y + 120;
        int dist_y = y + 195;// 175;
        disp_size = 60;
        if (no_radar) {
            if (stop_dist > 0.5 && stopping) {
                if (stop_dist < 10.0) sprintf(str, "%.1f", stop_dist);
                else sprintf(str, "%.0f", stop_dist);
                ui_draw_text(s, x, dist_y, str, disp_size, COLOR_WHITE, BOLD);
            }
            else if (longActiveUser > 0 && (stopping || lp.getTrafficState() >= 1000)) {
                if (brake_hold || soft_hold) {
                    //drawTextWithColor(painter, x, dist_y, (brake_hold) ? "AUTOHOLD" : "SOFTHOLD", textColor);
                    sprintf(str, "%s", (brake_hold) ? "AUTOHOLD" : "SOFTHOLD");
                    ui_draw_text(s, x, dist_y, str, disp_size, COLOR_WHITE, BOLD);
                }
                else {
                    sprintf(str, "%s", (lp.getTrafficState() >= 1000) ? "신호오류" : "신호대기");
                    ui_draw_text(s, x, dist_y, str, disp_size, COLOR_WHITE, BOLD);
                }
            }
        }
        else if (disp_dist > 0.0) {
            if (disp_dist < 10.0) sprintf(str, "%.1f", disp_dist);
            else sprintf(str, "%.0f", disp_dist);
            ui_draw_text(s, x, dist_y, str, disp_size, COLOR_WHITE, BOLD);
        }
    }
    if (s->show_path_end) {
        //if (path_y < s->fb_h - 200) ui_fill_rect(s->vg, { path_x - path_width / 2, path_y, path_width, -10 }, (radar_rel_speed > -0.1) ? COLOR_GREEN : COLOR_RED, 5);
        //if (path_y < s->fb_h - 200) ui_fill_rect(s->vg, { path_x - path_width / 2, path_y, path_width, -10 }, no_radar?COLOR_ORANGE:radar_detected?COLOR_RED:COLOR_BLUE, 5);

        float px[7], py[7];
        px[0] = path_x - path_width / 2;
        px[1] = path_x + path_width / 2;
        px[2] = path_x + path_width / 2;
        px[3] = path_x + 20;
        px[4] = path_x;
        px[5] = path_x - 20;
        px[6] = path_x - path_width / 2;
        py[0] = path_y;
        py[1] = path_y;
        py[2] = path_y - 7;
        py[3] = path_y - 17;
        py[4] = path_y - 0;
        py[5] = path_y - 17;
        py[6] = path_y - 7;
        NVGcolor  pcolor = no_radar ? ((trafficMode == 1)?COLOR_RED:COLOR_GREEN) : radar_detected ? COLOR_RED : COLOR_BLUE;
        ui_draw_line2(s, px, py, 7, &pcolor, nullptr, 3.0f);
        if (s->show_path_end > 0 && disp_dist > 0.0) {
            px[0] = path_x - path_width / 2 - 10;
            px[1] = px[0] + path_width + 20;
            px[2] = px[1];
            px[3] = px[0];
            py[0] = path_y + 5;
            py[1] = py[0];
            py[2] = py[1] - path_width * 0.8;
            py[3] = py[2];
            NVGcolor color2 = COLOR_BLACK_ALPHA(20);
            ui_draw_line2(s, px, py, 4, &color2, nullptr, 3.0f, radar_detected?COLOR_RED:COLOR_BLUE);

        }

    }

    // 타겟좌측 : 갭표시
    int myDrivingMode = controls_state.getMyDrivingMode();
    int active = controls_state.getActive();
    //const auto lp = sm["longitudinalPlan"].getLongitudinalPlan();
    float gap = lp.getCruiseGap();
    float tFollow = lp.getTFollow();
    int gap1 = controls_state.getLongCruiseGap(); // car_state.getCruiseGap();
#ifdef __TEST
    myDrivingMode = 3;
#endif
    char strDrivingMode[128];
    switch (myDrivingMode)
    {
    case 0: strcpy(strDrivingMode,"GAP"); break;
    case 1: strcpy(strDrivingMode, "연비"); break;// "연비"; break;
    case 2: strcpy(strDrivingMode, "안전"); break;// "안전"; break;
    case 3: strcpy(strDrivingMode, "일반"); break;// "일반"; break;
    case 4: strcpy(strDrivingMode, "고속"); break;// "고속"; break;
    }

    int dxGap = -128 - 10 - 40;
    if (!carrot_hud && s->show_gap_info >= 0) {
        sprintf(str, "%.2f", tFollow);
        ui_draw_text(s, x + dxGap + 15 - 60, y + 120.0, str, 30, COLOR_WHITE, BOLD);
        sprintf(str, "%.0fM", tFollow * v_ego + 6.0);
        ui_draw_text(s, x + dxGap + 15 - 60, y + 155.0, str, 30, COLOR_WHITE, BOLD);

        ui_draw_text(s, x + dxGap + 15, y + 120.0, strDrivingMode, 30, COLOR_WHITE, BOLD);
    }
    static int _myDrivingMode = 0;
    if (!carrot_hud && _myDrivingMode != myDrivingMode) ui_draw_text_a(s, x + dxGap + 15, y + 120, strDrivingMode, 30, COLOR_WHITE, BOLD);
    _myDrivingMode = myDrivingMode;
    dxGap -= 60;
    if (!carrot_hud && s->show_gap_info > 0) {
#ifdef __TEST
        static int _gap = 0;
        _gap += 10;
        if (_gap > 400) _gap = 0;
        else if (_gap < 100) gap = 1;
        else if (_gap < 200) gap = 2;
        else if (_gap < 300) gap = 3;
        else gap = 4;

        static int _preGap = 0;
        if (_preGap != gap) {
            sprintf(str, "%d", (int)gap);
            ui_draw_text_a(s, x + dxGap + 15 + 60, y + 60, str, 50, COLOR_WHITE, BOLD);
        }
        _preGap = gap;
#endif
        ui_fill_rect(s->vg, { x + dxGap - 2, (int)(y + 5 + 64), 40, -(int)(std::clamp((float)gap, 0.0f, 4.0f) / 4. * 64) }, COLOR_GREEN, 0);
        ui_draw_rect(s->vg, { x + dxGap, y + 5, 40, 64 / 4 }, COLOR_WHITE, 4, 0);
        ui_draw_rect(s->vg, { x + dxGap, (int)(y + 5 + 64 * 1 / 4.), 40, 64 / 4 }, COLOR_WHITE, 4, 0);
        ui_draw_rect(s->vg, { x + dxGap, (int)(y + 5 + 64 * 2 / 4.), 40, 64 / 4 }, COLOR_WHITE, 4, 0);
        ui_draw_rect(s->vg, { x + dxGap, (int)(y + 5 + 64 * 3 / 4.), 40, 64 / 4 }, COLOR_WHITE, 4, 0);
        ui_draw_text(s, x + dxGap + 20, y+90, "GAP", 25, COLOR_WHITE, BOLD);
    }
    // 갭정보표시 중앙위
    sprintf(str, "%d", gap1);
    if (!carrot_hud && s->show_gap_info >= 0) {
        ui_draw_text(s, x + dxGap + 15 + 60, y + 60, str, 50, COLOR_WHITE, BOLD);
    }
    static int _gap1 = 0;
    if(!carrot_hud && _gap1 != gap1) ui_draw_text_a(s, x + dxGap + 15 + 60, y + 60, str, 50, COLOR_WHITE, BOLD);
    _gap1 = gap1;
    // 타겟하단: 롱컨상태표시
    if (true) {
        auto xState = lp.getXState();
        QString qstr;
        if (brake_hold) qstr = "AUTOHOLD";
        else if (active < 0) {
            qstr = "NOT ACTIVE";
        }
        else if (longActiveUser > 0) {
            if (xState == cereal::LongitudinalPlan::XState::E2E_STOP) qstr = tr("SIGN DETECTED");
            else if (xState == cereal::LongitudinalPlan::XState::SOFT_HOLD) qstr = "SOFTHOLD";
            else if (xState == cereal::LongitudinalPlan::XState::LEAD) qstr = "LEAD";
            else if (xState == cereal::LongitudinalPlan::XState::E2E_CRUISE) qstr = (v_ego_kph < 80) ? tr("E2ECRUISE") : tr("CRUISE");
            else if (xState == cereal::LongitudinalPlan::XState::E2E_CRUISE_PREPARE) qstr = "E2EPREPARE";
            else if (xState == cereal::LongitudinalPlan::XState::CRUISE) qstr = tr("CRUISE");
            else qstr = "UNKNOWN";
        }
        else {
            if (longActiveUserReady > 0) {
                if (xState == cereal::LongitudinalPlan::XState::SOFT_HOLD) qstr = "SOFTHOLD";
                else qstr = tr("CRUISE READY");
            }
            else qstr = tr("MANUAL");
        }
        //if(brake_valid) ui_draw_text(s, x, y + 175, qstr.toStdString().c_str(), 40, COLOR_WHITE, BOLD, 1.0, 3.0, COLOR_RED, COLOR_RED);
        //else            ui_draw_text(s, x, y + 175, qstr.toStdString().c_str(), 40, COLOR_WHITE, BOLD, 1.0, 3.0, COLOR_BLACK, COLOR_BLACK);
        static QString _qstr = "";        
        if (longActiveUser > 0) {
            if (xState == cereal::LongitudinalPlan::XState::SOFT_HOLD) qstr = "SOFTHOLD";
            else qstr = "CRUISE";
        }
        else qstr = "MANUAL";
        if (qstr != _qstr) ui_draw_text_a(s, x, y + 175, qstr.toStdString().c_str(), 40, COLOR_WHITE, BOLD);
        _qstr = qstr;

    }
    // Accel표시
    float accel = car_state.getAEgo();
    int dx = 128 + 10;
#ifdef __TEST
    static float accel1 = 0.0;
    accel1 += 0.2;
    if (accel1 > 2.5) accel1 = -2.5;
    accel = accel1;
#endif
    if (s->show_accel > 0) {
        ui_draw_rect(s->vg, { x + dx, y+5, 40, 128 }, COLOR_WHITE, 4, 0);
        ui_fill_rect(s->vg, { x + dx + 2, y + 64 + 5, 36, -(int)(std::clamp((float)accel, -2.0f, 2.0f) / 2. * 64) }, (accel>=0.0)?COLOR_YELLOW:COLOR_RED, 0);
        ui_draw_text(s, x + dx + 20, y + 160, "ACC", 25, COLOR_WHITE, BOLD);
    }

    // RPM표시
    float engineRpm = car_state.getEngineRpm();
    float motorRpm = car_state.getMotorRpm();
#ifdef __TEST
    static float engineRpm1 = 0.0;
    engineRpm1 += 100.0;
    if (engineRpm1 > 4000.0) engineRpm1 = 0.0;
    motorRpm = engineRpm1;
#endif
    if (s->show_accel > 1) {
        dx += 60;
        //str.sprintf("%s: %.0f CHARGE: %.0f%%", (motorRpm > 0.0) ? "MOTOR" : "RPM", (motorRpm > 0.0) ? motorRpm : engineRpm, car_state.getChargeMeter());
        //drawTextWithColor(p, width() - 350, 80, str, textColor);
        //painter.setPen(Qt::NoPen);
        ui_draw_rect(s->vg, { x + dx, y + 5, 40, 128 }, COLOR_WHITE, 4, 0);
        ui_fill_rect(s->vg, { x + dx + 2, y + 128 + 5, 36, -(int)(std::clamp((float)engineRpm>0.0?engineRpm:motorRpm, 0.0f, 4000.0f) / 4000. * 128.0) }, (engineRpm> 0.0) ? COLOR_BLUE : COLOR_GREEN, 0);
        ui_draw_text(s, x + dx + 20, y + 160, "RPM", 25, COLOR_WHITE, BOLD);
    }
    if (s->show_mode == 3) {
        y = -100;
        if (s->fb_w < 1200) x = 650;
        else x = 950;
    }
    else if (s->show_mode == 4) {
        y = 380;
        x = 150;
    }
    else if (s->show_mode == 5) {
        y = 620;
        x = 400;
    }
    else if (carrot_hud) {  // speed anchor (x, y + 270) = carrot (140, fb_h - 230)
        x = 140;
        y = s->fb_h - 230 - 270;
    }

    QString navText = "";
    // 속도표시
    if (true) {
        const auto road_limit_speed = sm["roadLimitSpeed"].getRoadLimitSpeed();
        const auto car_params = sm["carParams"].getCarParams();

        //bool is_metric = s->scene.is_metric;
        //bool long_control = 1;// scc_smoother.getLongControl();

        // kph
        float applyMaxSpeed = controls_state.getVCruiseOut();// scc_smoother.getApplyMaxSpeed();
        float cruiseMaxSpeed = controls_state.getVCruiseCluster();// scc_smoother.getCruiseMaxSpeed();
        float curveSpeed = controls_state.getCurveSpeed();
        bool speedCtrlActive = false;
        if (curveSpeed < 0) {
            speedCtrlActive = true;
            curveSpeed = -curveSpeed;
        }
        
        //float xCruiseTarget = lp.getXCruiseTarget() * 3.6;

        //bool is_cruise_set = (cruiseMaxSpeed > 0 && cruiseMaxSpeed < 255);
        //bool is_cruise_set = (applyMaxSpeed > 0 && applyMaxSpeed < 255);
        //int longActiveUser = controls_state.getLongActiveUser();
        int longOverride = car_control.getLongOverride();

        int sccBus = (int)car_params.getSccBus();

        int enabled = controls_state.getEnabled();

        int activeNDA = road_limit_speed.getActive();
        int roadLimitSpeed = road_limit_speed.getRoadLimitSpeed();
        int camLimitSpeed = road_limit_speed.getCamLimitSpeed();
        int camLimitSpeedLeftDist = road_limit_speed.getCamLimitSpeedLeftDist();
        int sectionLimitSpeed = road_limit_speed.getSectionLimitSpeed();
        int sectionLeftDist = road_limit_speed.getSectionLeftDist();
        int camType = road_limit_speed.getCamType();

        int limit_speed = 0;
        int left_dist = 0;

        if (camLimitSpeed > 0 && camLimitSpeedLeftDist > 0) {
            limit_speed = camLimitSpeed;
            left_dist = camLimitSpeedLeftDist;
        }
        else if (sectionLimitSpeed > 0 && sectionLeftDist > 0) {
            limit_speed = sectionLimitSpeed;
            left_dist = sectionLeftDist;
        }
        //const auto road_limit_speed = sm["roadLimitSpeed"].getRoadLimitSpeed();
        int xTurnInfo = road_limit_speed.getXTurnInfo();
        int xDistToTurn = road_limit_speed.getXDistToTurn();
        int xSpdDist = road_limit_speed.getXSpdDist();
        int xSpdLimit = road_limit_speed.getXSpdLimit();
        int xSignType = road_limit_speed.getXSignType();
        navText = QString::fromStdString(road_limit_speed.getXRoadName());
#ifdef __TEST
        xTurnInfo = 2;
        xDistToTurn = 120;
#endif
        if (limit_speed > 0); 
        else if (xSpdLimit > 0 && xSpdDist > 0) {
            limit_speed = xSpdLimit;
            left_dist = xSpdDist;
        }
        else if (xTurnInfo >= 0) {
            left_dist = xDistToTurn;
        }

        //sprintf(str, "%.1f/%.1f %s(%s)", distance, distance_remaining, type.length() ? type.toStdString().c_str() : "", modifier.length() ? modifier.toStdString().c_str() : "");


        int radar_tracks = Params().getBool("EnableRadarTracks");
        //QString nda_mode_str = QString::fromStdString(Params().get("AutoNaviSpeedCtrl"));
        //int nda_mode = nda_mode_str.toInt();

        sprintf(str, "%s %s %s", (sccBus) ? "SCC2" : "", (activeNDA > 0) ? "NDA" : "", (radar_tracks) ? "RadarTracks" : "");

        //float accel = car_state.getAEgo();
#ifdef __TEST
        static int _ff = 0;
        if (_ff++ > 100) _ff = 0;
        if (_ff > 50) {
            limit_speed = 110;
            left_dist = _ff * 100;
        }
        else {
            roadLimitSpeed = 110;
        }
        cur_speed = 123;
#endif

        if (accel >= 0) {
            //int a = (int)(255.f - (180.f * (accel / 2.f)));
            //a = std::min(a, 255);
            //a = std::max(a, 80);
            //color = QColor(a, a, 255, 230);
        }
        else {
            int a = (int)(255.f - (255.f * (-accel / 3.f)));
            a = std::min(a, 255);
            a = std::max(a, 60);
            //color = QColor(255, a, a, 255);
        }
        if (s->show_conn_info) {
            //ui_draw_text(s, strlen(str) / 2 * 35 / 2 + 50,40, str, 35, COLOR_WHITE, BOLD);
            int hda_speedLimit = car_state.getSpeedLimit(); 
            int hda_speedLimitDistance = car_state.getSpeedLimitDistance();
            int naviCluster = (int)car_params.getNaviCluster();
            if (sccBus) ui_draw_image(s, { 30, 20, 120, 54 }, "ic_scc2", 1.0f);
            if (activeNDA >= 200) ui_draw_image(s, { 30 + 135, 20, 120, 54 }, "ic_apn", 1.0f);
            else if (hda_speedLimit > 0 && hda_speedLimitDistance > 0) ui_draw_image(s, { 30 + 135, 20, 120, 54 }, "ic_hda", 1.0f);
            else if (activeNDA >= 100) ui_draw_image(s, { 30 + 135, 20, 120, 54 }, "ic_apm", 1.0f);
            else if (activeNDA % 100 > 0) ui_draw_image(s, { 30 + 135, 20, 120, 54 }, "ic_nda", 1.0f);
            else if (naviCluster > 0) ui_draw_image(s, { 30 + 135, 20, 120, 54 }, "ic_hda", 1.0f);
            if (radar_tracks) ui_draw_image(s, { 30 + 135 * 2, 20, 240, 54 }, "ic_radartracks", 1.0f);
            // GPS line right after the last badge drawn in this row
            const bool nav_badge = activeNDA > 0 || (hda_speedLimit > 0 && hda_speedLimitDistance > 0) || naviCluster > 0;
            g_gps_x = radar_tracks ? 30 + 135 * 2 + 255 : nav_badge ? 30 + 135 * 2 : sccBus ? 30 + 135 : 30;
        }

        int bx = x;
        int by = y + 270;

        // carrot box goes under everything else in the panel (traffic light, speed, ...)
        const bool speed_bump = (xSignType == 124 || camType == 22);
        const int carrot_cam_limit = (limit_speed > 0 && !speed_bump) ? limit_speed : 0;
        if (carrot_hud) carrot_hud_background(s, bx, by, carrot_cam_limit > 0);

        if (s->show_mode == 4 || s->show_mode == 5 || carrot_hud) {
            if (trafficMode == 1) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2 + 270, icon_size, icon_size }, "ic_traffic_red", 1.0f);
            else if (trafficMode == 2) ui_draw_image(s, { x - icon_size / 2, y - icon_size / 2 + 270, icon_size, icon_size }, "ic_traffic_green", 1.0f);
        }

        char speed[128];
        sprintf(speed, "%.0f", cur_speed);
        ui_draw_text(s, bx, by + 50, speed, 120, COLOR_WHITE, BOLD, 3.0f, 8.0f);
        ui_draw_image(s, { bx - 100, by - 60, 350, 150 }, "ic_speed_bg", 1.0f);

        //color = QColor(255, 255, 255, 255);
#ifdef __TEST
        cruiseMaxSpeed = 110;
        enabled = true;
        longActiveUser = 2;
        applyMaxSpeed = 109;
        curveSpeed = 111;
#endif
        static char _speed_str[128] = "";
        if (enabled && (longActiveUser > 0 || (longOverride && blinkerOn))) {
            sprintf(str, "%d", (int)(cruiseMaxSpeed * (s->scene.is_metric?1.0:KM_TO_MILE) + 0.5));
            if (strcmp(_speed_str, str)) ui_draw_text_a(s, bx + 170, by + 15, str, 60, COLOR_GREEN, BOLD);
            strcpy(_speed_str, str);
        }
        else strcpy(str,"--");
        ui_draw_text(s, bx + 170, by + 15, str, 60, COLOR_GREEN, BOLD, 1.0, 5.0, COLOR_BLACK, COLOR_BLACK);
        sprintf(str, "%d", (int)(applyMaxSpeed * (s->scene.is_metric ? 1.0 : KM_TO_MILE) + 0.5));
        const bool apply_differs = enabled && longActiveUser > 0 && applyMaxSpeed > 0 && applyMaxSpeed != cruiseMaxSpeed;
        if (carrot_hud) {
            // carrot: the speed actually applied, in ochre, with why it differs from the set speed
            if (apply_differs) {
                const char* source = (applyMaxSpeed > cruiseMaxSpeed) ? "eco" :
                                     (carrot_cam_limit > 0) ? "cam" :
                                     (speed_bump && limit_speed > 0) ? "bump" :
                                     (speedCtrlActive) ? "curve" : "decel";
                ui_draw_text(s, bx + 250, by - 50, str, 50, COLOR_OCHRE, BOLD, 1.0, 5.0, COLOR_BLACK, COLOR_BLACK);
                ui_draw_text(s, bx + 250, by - 100, source, 30, COLOR_OCHRE, BOLD, 1.0, 5.0, COLOR_BLACK, COLOR_BLACK);
            }
        }
        else if (apply_differs) {
            ui_draw_text(s, bx + 250, by - 50, str, 50, COLOR_GREEN, BOLD, 1.0, 5.0, COLOR_BLACK, COLOR_BLACK);
        }
        if (!carrot_hud) {
            if (enabled && curveSpeed > 0 && curveSpeed < 150) {
                sprintf(str, "%d", (int)(curveSpeed* (s->scene.is_metric ? 1.0 : KM_TO_MILE) + 0.5));
                ui_draw_text(s, bx + 140, by + 110, str, 50, (speedCtrlActive)?COLOR_RED:COLOR_YELLOW, BOLD, 1.0, 5.0, COLOR_BLACK, COLOR_BLACK);
            }
        }

        if (carrot_hud) {
            carrot_hud_lower(s, bx, by, myDrivingMode, gap1, carrot_cam_limit,
                             (roadLimitSpeed > 0 && roadLimitSpeed < 200) ? roadLimitSpeed : 0, cur_speed, activeNDA);
        }

        bx = x - 200;
        by = y + 250;
        if (carrot_hud) {  // turn / bump signs go right of the carrot box
            bx = 620;
            by = s->fb_h - 300;
        }
        else if (s->show_mode == 4) {
            bx = 150;
            by = 380;
        }
        else if (s->show_mode == 5) {
            bx = 100;
            by = 650;
        }
        if (left_dist > 0) {
            if (left_dist < 1000) sprintf(str, "%d m", left_dist);
            else  sprintf(str, "%.1f km", left_dist / 1000.f);
            ui_draw_text(s, bx, by + 120, str, 40, COLOR_WHITE, BOLD);
        }

        if (limit_speed > 0 && !(carrot_hud && !speed_bump)) {  // carrot box shows the cam limit itself
            if (xSignType == 124 || camType == 22) {
                ui_draw_image(s, { bx - 60, by - 50, 120, 150 }, "ic_speed_bump", 1.0f);
            }
            else {
                nvgBeginPath(s->vg);
                nvgCircle(s->vg, bx, by, 140 / 2);
                nvgFillColor(s->vg, COLOR_WHITE);
                nvgFill(s->vg);
                nvgBeginPath(s->vg);
                nvgCircle(s->vg, bx, by, 130 / 2);
                nvgFillColor(s->vg, COLOR_RED);
                nvgFill(s->vg);
                nvgBeginPath(s->vg);
                nvgCircle(s->vg, bx, by, 110 / 2);
                nvgFillColor(s->vg, COLOR_WHITE);
                nvgFill(s->vg);
                sprintf(str, "%d", limit_speed);
                ui_draw_text(s, bx, by + 25, str, 60, COLOR_BLACK, BOLD, 0.0f, 0.0f);
            }
            if (false && left_dist > 0) {
                if (left_dist < 1000) sprintf(str, "%d m", left_dist);
                else  sprintf(str, "%.1f km", left_dist / 1000.f);
                ui_draw_text(s, bx, by + 120, str, 40, COLOR_WHITE, BOLD);
            }
        }
        else if (xTurnInfo >= 0) {
            switch (xTurnInfo) {
            case 1: ui_draw_image(s, { bx - icon_size / 2, by - icon_size / 2, icon_size, icon_size }, "ic_turn_l", 1.0f); break;
            case 2: ui_draw_image(s, { bx - icon_size / 2, by - icon_size / 2, icon_size, icon_size }, "ic_turn_r", 1.0f); break;
            case 3: ui_draw_image(s, { bx - icon_size / 2, by - icon_size / 2, icon_size, icon_size }, "ic_lane_change_l", 1.0f); break;
            case 4: ui_draw_image(s, { bx - icon_size / 2, by - icon_size / 2, icon_size, icon_size }, "ic_lane_change_r", 1.0f); break;
            case 5: ui_draw_text(s, bx, by + 20, "U턴", 35, COLOR_WHITE, BOLD, 0.0f, 0.0f); break;
            case 35: ui_draw_text(s, bx, by + 20, "좌측고가 진입", 35, COLOR_WHITE, BOLD, 0.0f, 0.0f); break;
            case 43: ui_draw_text(s, bx, by + 20, "지하차도 우측", 35, COLOR_WHITE, BOLD, 0.0f, 0.0f); break;
            case 48: ui_draw_text(s, bx, by + 20, "휴게소", 35, COLOR_WHITE, BOLD, 0.0f, 0.0f); break;
            default:
                sprintf(str, "%d", xTurnInfo);
                ui_draw_text(s, bx, by + 20, str, 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);
                break;
            }
        }
        else if (!carrot_hud && roadLimitSpeed > 0 && roadLimitSpeed < 200) {
            ui_draw_image(s, { bx - 60, by - 50, 120, 150 }, "ic_road_speed", 1.0f);
            sprintf(str, "%d", roadLimitSpeed);
            ui_draw_text(s, bx, by + 75, str, 50, COLOR_BLACK, BOLD, 0.0f, 0.0f);
        }
    }
    // Tpms...
    if (s->show_tpms) {
        if (carrot_hud) {  // the carrot box owns the lower-left corner: draw tires at the lower right
            nvgSave(s->vg);
            nvgTranslate(s->vg, s->fb_w - 300, -60);
        }
        int bx = (192 - 24) / 2 + (bdr_s * 2);
        int by = s->fb_h - 280 / 2;
        auto tpms = car_state.getTpms();
        float fl = tpms.getFl();
        float fr = tpms.getFr();
        float rl = tpms.getRl();
        float rr = tpms.getRr();
#ifdef __TEST
        fl = 25;
        fr = 28;
        rl = 32;
        rr = 44;
        //s->show_dm_info = 0;
#endif
        if (s->show_dm_info < 1) {
            int sx = 100;
            int sy = 220;
            bx = 120;
            by = s->fb_h - 100;
            ui_draw_image(s, { bx - sx / 2, by - sy / 2, sx, sy }, "ic_tire", 1.0f);
            //QColor tpmsColor = get_tpms_color(fl);
            //drawTextWithColor(painter, bx - 80, by - 50, get_tpms_text(fl), tpmsColor);
            //tpmsColor = get_tpms_color(fr);
            //drawTextWithColor(painter, bx + 80, by - 50, get_tpms_text(fr), tpmsColor);
            //tpmsColor = get_tpms_color(rl);
            //drawTextWithColor(painter, bx - 80, by + 75, get_tpms_text(rl), tpmsColor);
            //tpmsColor = get_tpms_color(rr);
            //drawTextWithColor(painter, bx + 80, by + 75, get_tpms_text(rr), tpmsColor);
            ui_draw_text(s, bx - 80, by - 50, get_tpms_text(fl), 38, get_tpms_color(fl), BOLD);
            ui_draw_text(s, bx + 80, by - 50, get_tpms_text(fr), 38, get_tpms_color(fr), BOLD);
            ui_draw_text(s, bx - 80, by + 75, get_tpms_text(rl), 38, get_tpms_color(rl), BOLD);
            ui_draw_text(s, bx + 80, by + 75, get_tpms_text(rr), 38, get_tpms_color(rr), BOLD);
        }
        else {
            //int center_x = bx - 30;
            //int center_y = by - 0;
            //int marginX = (int)(rcFont.width() * 3.2f);
            //int marginY = (int)((footer_h / 2 - rcFont.height()) * 0.6f);
            //drawText2(painter, center_x - marginX, center_y - marginY - rcFont.height(), Qt::AlignRight, get_tpms_text(fl), get_tpms_color(fl));
            //drawText2(painter, center_x + marginX, center_y - marginY - rcFont.height(), Qt::AlignLeft, get_tpms_text(fr), get_tpms_color(fr));
            //drawText2(painter, center_x - marginX, center_y + marginY, Qt::AlignRight, get_tpms_text(rl), get_tpms_color(rl));
            //drawText2(painter, center_x + marginX, center_y + marginY, Qt::AlignLeft, get_tpms_text(rr), get_tpms_color(rr));
            ui_draw_text(s, bx - 90, by - 55, get_tpms_text(fl), 38, get_tpms_color(fl), BOLD);
            ui_draw_text(s, bx + 90, by - 55, get_tpms_text(fr), 38, get_tpms_color(fr), BOLD);
            ui_draw_text(s, bx - 90, by + 80, get_tpms_text(rl), 38, get_tpms_color(rl), BOLD);
            ui_draw_text(s, bx + 90, by + 80, get_tpms_text(rr), 38, get_tpms_color(rr), BOLD);
        }
        if (carrot_hud) nvgRestore(s->vg);
    }
    // 시간표시
    if (s->show_datetime) {
        time_t now = time(nullptr);
        struct tm* local = localtime(&now);

        int nav_y = 170 + 40;

        if (s->show_datetime == 1 || s->show_datetime == 2) {
            strftime(str, sizeof(str), "%H:%M", local);
            ui_draw_text(s, 170, 170, str, 100, COLOR_WHITE, BOLD, 3.0f, 8.0f);

        }
        if (s->show_datetime == 1 || s->show_datetime == 3) {
            strftime(str, sizeof(str), "%m-%d-%a", local);
            ui_draw_text(s, 170, 170+70, str, 60, COLOR_WHITE, BOLD, 3.0f, 8.0f);
            nav_y += 70;
        }
        nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        ui_draw_text(s, 50, nav_y, navText.toStdString().c_str(), 35, COLOR_WHITE, BOLD, 3.0f, 8.0f);
        nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
    }
    v_ego_kph = v_ego_kph;
    brake_valid = brake_valid;
    longActiveUserReady = longActiveUserReady;
}
void DrawApilot::drawDeviceState(UIState* s, bool show) {
    const SubMaster& sm = *(s->sm);
    auto deviceState = sm["deviceState"].getDeviceState();
    char  str[128];
    QString qstr;
    const auto freeSpacePercent = deviceState.getFreeSpacePercent();
    const auto memoryUsagePercent = deviceState.getMemoryUsagePercent();

    const auto cpuTempC = deviceState.getCpuTempC();
    //const auto gpuTempC = deviceState.getGpuTempC();
    float ambientTemp = deviceState.getAmbientTempC();
    float cpuTemp = 0.f;
    //float gpuTemp = 0.f;

    nvgTextAlign(s->vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

    if (std::size(cpuTempC) > 0) {
        for (int i = 0; i < std::size(cpuTempC); i++) {
            cpuTemp += cpuTempC[i];
        }
        cpuTemp = cpuTemp / (float)std::size(cpuTempC);
    }
    auto car_state = sm["carState"].getCarState();
    //const cereal::ModelDataV2::Reader& model = sm["modelV2"].getModelV2();
    sprintf(str, "MEM: %d%% STORAGE: %.0f%% CPU: %.0f°C AMBIENT: %.0f°C", memoryUsagePercent, freeSpacePercent, cpuTemp, ambientTemp);
    int r = interp<float>(cpuTemp, { 50.f, 90.f }, { 200.f, 255.f }, false);
    int g = interp<float>(cpuTemp, { 50.f, 90.f }, { 255.f, 200.f }, false);
    NVGcolor textColor = nvgRGBA(r, g, 200, 255);
    // C2 port: the top right corner is kept empty; the carrot HUD box shows CPU temperature, CPU use and FPS
    if (false && s->fb_w > 1200 && show) {
        ui_draw_text(s, s->fb_w - 20, 35, str, 35, textColor, BOLD);
        float engineRpm = car_state.getEngineRpm();
        float motorRpm = car_state.getMotorRpm();
        sprintf(str, "FPS: %d, %s: %.0f CHARGE: %.0f%%                      ", g_fps, (motorRpm > 0.0) ? "MOTOR" : "RPM", (motorRpm > 0.0) ? motorRpm : engineRpm, car_state.getChargeMeter());
        ui_draw_text(s, s->fb_w - 20, 90, str, 35, textColor, BOLD);
    }
    // C2 port: no IP address in the bottom right corner, the sidebar already shows it
}
void DrawApilot::drawDebugText(UIState* s, bool show) {
    if (s->fb_w < 1200) return;
    if (show == false) return;
    const SubMaster& sm = *(s->sm);
    char  str[128];
    QString qstr;

    nvgTextAlign(s->vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

    int y = 350, dy = 40;

    const int text_x = s->fb_w - 20;
    const auto live_torque_params = sm["liveTorqueParameters"].getLiveTorqueParameters();
    
    sprintf(str, "LT[%.0f]:%s (%.4f/%.4f)", live_torque_params.getTotalBucketPoints(), live_torque_params.getLiveValid() ? "ON" : "OFF", live_torque_params.getLatAccelFactorFiltered(), live_torque_params.getFrictionCoefficientFiltered());
    ui_draw_text(s, text_x, y, str, 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);

    qstr = QString::fromStdString(live_torque_params.getDebugText().cStr());
    y += dy;
    ui_draw_text(s, text_x, y, qstr.toStdString().c_str(), 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);

    const auto lp = sm["longitudinalPlan"].getLongitudinalPlan();
    qstr = QString::fromStdString(lp.getDebugLongText1().cStr());
    y += dy;
    ui_draw_text(s, text_x, y, qstr.toStdString().c_str(), 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);
    const auto live_params = sm["liveParameters"].getLiveParameters();
    float   liveSteerRatio = live_params.getSteerRatio();
    sprintf(str, "LiveSR = %.2f", liveSteerRatio);
    y += dy;
    ui_draw_text(s, text_x, y, str, 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);

    auto controls_state = sm["controlsState"].getControlsState();
    qstr = QString::fromStdString(controls_state.getDebugText1().cStr());
    y += dy;
    ui_draw_text(s, text_x, y, qstr.toStdString().c_str(), 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);
    qstr = QString::fromStdString(controls_state.getDebugText2().cStr());
    y += dy;
    ui_draw_text(s, text_x, y, qstr.toStdString().c_str(), 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);
    const auto road_limit_speed = sm["roadLimitSpeed"].getRoadLimitSpeed();
    int xTurnInfo = road_limit_speed.getXTurnInfo();
    int xDistToTurn = road_limit_speed.getXDistToTurn();
    int xSpdDist = road_limit_speed.getXSpdDist();
    int xSpdLimit = road_limit_speed.getXSpdLimit();
    int xSignType = road_limit_speed.getXSignType();
    int xRoadSignType = road_limit_speed.getXRoadSignType();
    int xRoadLimitSpeed = road_limit_speed.getXRoadLimitSpeed();

    auto lateralPlan = sm["lateralPlan"].getLateralPlan();
    float laneWidth = lateralPlan.getLaneWidth();
    int roadEdgeStat = lateralPlan.getRoadEdgeStat();
    QString latDebugText = QString::fromStdString(lateralPlan.getLatDebugText());


    sprintf(str, "Mappy: Turn(%d,%d), Spd(%d,%d),Sign(%d), Road(%d,%d), LW:%.1f", xTurnInfo, xDistToTurn, xSpdDist, xSpdLimit, xSignType, xRoadSignType, xRoadLimitSpeed, laneWidth);
    y += dy;
    ui_draw_text(s, text_x, y, str, 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);
    sprintf(str, "Edge(%d), %s",  roadEdgeStat, latDebugText.toStdString().c_str());
    y += dy;
    ui_draw_text(s, text_x, y, str, 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);

    //p.drawText(text_x, y + 160, QString::fromStdString(controls_state.getDebugText2().cStr()));
    //p.drawText(text_x, y + 240, QString::fromStdString(controls_state.getDebugText1().cStr()));

    auto car_control = sm["carControl"].getCarControl();
    qstr = QString::fromStdString(car_control.getDebugTextCC().cStr());
    y += dy;
    ui_draw_text(s, text_x, y, qstr.toStdString().c_str(), 35, COLOR_WHITE, BOLD, 0.0f, 0.0f);

}
DrawApilot::DrawApilot() {

}
DrawApilot* drawApilot;
Alert alert;
NVGcolor alert_color;
// GPS status line at the bottom right, left of the Wi-Fi address: fix state, 4 signal bars, satellites seen, accuracy.
// Signal strength = mean C/N0 of the 4 strongest satellites from ublox RXM-RAWX (ubloxGnss.measurementReport).
static void ui_draw_gps_info(const UIState* s) {
    if (!s->show_gps_info) return;
    SubMaster& sm = *(s->sm);
    const UIScene& scene = s->scene;
    const int gps_sats = scene.gps_sats, gps_strong = scene.gps_strong, gps_cno = scene.gps_cno;
    const bool gps_antenna_bad = scene.gps_antenna_bad;
    const bool gnss_alive = sm.alive("ubloxGnss");

    const bool loc_alive = sm.alive("gpsLocationExternal");
    const auto loc = sm["gpsLocationExternal"].getGpsLocationExternal();
    const bool fixed = loc_alive && (loc.getFlags() & 1);
    const float acc = loc.getAccuracy();

    const char* state;
    NVGcolor color;
    if (!gnss_alive && !loc_alive) { state = "꺼짐"; color = nvgRGBA(170, 170, 170, 255); }
    else if (gps_antenna_bad) { state = "안테나 이상"; color = COLOR_RED; }
    else if (!fixed) { state = "찾는 중"; color = COLOR_YELLOW; }
    else if (acc <= 5.0f) { state = "잡힘"; color = nvgRGBA(0, 230, 0, 255); }
    else if (acc <= 15.0f) { state = "잡힘"; color = COLOR_YELLOW; }
    else { state = "약함"; color = COLOR_OCHRE; }
    if (s->show_mode == 7) {  // the optech HUD's own soft grey / red / green / amber
        if (!gnss_alive && !loc_alive) color = nvgRGBA(180, 178, 169, 255);
        else if (gps_antenna_bad) color = nvgRGBA(240, 149, 149, 255);
        else if (fixed && acc <= 5.0f) color = nvgRGBA(151, 196, 89, 255);
        else color = nvgRGBA(250, 199, 117, 255);
    }

    // no middle dot / degree sign: the font has neither and stops drawing the string there
    char str[96];
    if (s->show_mode == 7) snprintf(str, sizeof(str), "GPS %s", state);  // optech HUD: short, the row is shared with the speed
    else if (fixed) snprintf(str, sizeof(str), "GPS %s  %s %d%s(%s %d)  %s %.0fm", state, "위성", gps_sats, "개", "강", gps_strong, "오차", acc);
    else if (gnss_alive) snprintf(str, sizeof(str), "GPS %s  %s %d%s(%s %d)", state, "위성", gps_sats, "개", "강", gps_strong);
    else snprintf(str, sizeof(str), "GPS %s", state);

    const int bars = gps_cno >= 35 ? 4 : gps_cno >= 30 ? 3 : gps_cno >= 25 ? 2 : gps_cno >= 20 ? 1 : 0;
    if (s->show_mode == 7) {
        // optech HUD: bottom-left row, right after the badges; shrink font only if needed so it never hits the centre speed
        const float bx = g_gps_x, by0 = s->fb_h - 20 - 54, bh = 54, cy = by0 + bh / 2;
        float fs = 30;
        const float bw = 8, gap = 4, barsw = 4 * bw + 3 * gap, maxbar = 30;
        nvgFontFace(s->vg, BOLD);
        nvgFontSize(s->vg, fs);
        float tb[4];
        nvgTextBounds(s->vg, 0, 0, str, NULL, tb);
        const float max_w = (s->fb_w / 2.f - 280.f) - bx;
        float w = 18 + barsw + 14 + (tb[2] - tb[0]) + 20;
        if (max_w > 100.f && w > max_w) {
            const float avail_tw = std::max(40.f, max_w - (18 + barsw + 14 + 20));
            if (tb[2] - tb[0] > avail_tw) {
                fs *= avail_tw / (tb[2] - tb[0]);
                nvgFontSize(s->vg, fs);
                nvgTextBounds(s->vg, 0, 0, str, NULL, tb);
            }
            w = 18 + barsw + 14 + (tb[2] - tb[0]) + 20;
        }
        carrot_round_box(s, bx, by0, w, bh, nvgRGBA(0, 0, 0, 110), nvgRGBA(136, 135, 128, 255), 14, 2);
        const float bottom = cy + maxbar / 2;
        for (int i = 0; i < 4; i++) {
            const float h = 12 + i * 6;
            nvgBeginPath(s->vg);
            nvgRoundedRect(s->vg, bx + 18 + i * (bw + gap), bottom - h, bw, h, 2);
            nvgFillColor(s->vg, i < bars ? color : nvgRGBA(255, 255, 255, 55));
            nvgFill(s->vg);
        }
        optech_text_vc(s, bx + 18 + barsw + 14, cy, str, fs, color, NVG_ALIGN_LEFT);
        return;
    }

    // top left, in the badge row after SCC2 and the navigation badge (drawLeadApilot sets g_gps_x past them)
    const float fsize = 32, by = 64;  // badges are 20..74 high
    const float bw = 9, gap = 4;
    for (int i = 0; i < 4; i++) {  // signal bars: <20 dB-Hz none, then one bar per 5 dB-Hz up to 35+
        const float bh = 8 + i * 6;
        nvgBeginPath(s->vg);
        nvgRect(s->vg, g_gps_x + i * (bw + gap), by - bh, bw, bh);
        nvgFillColor(s->vg, i < bars ? color : nvgRGBA(255, 255, 255, 60));
        nvgFill(s->vg);
        nvgStrokeColor(s->vg, COLOR_BLACK_ALPHA(200));
        nvgStrokeWidth(s->vg, 1.5f);
        nvgStroke(s->vg);
    }
    nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
    ui_draw_text(s, g_gps_x + 4 * (bw + gap) + 12, by, str, fsize, color, BOLD, 2.0f, 0.0f);
}

// optech 후측방 경고 (화면 탭 '후측방(BSD) 경고'): a big mark at the screen edge on the side of the car in the
// blind spot, so it can be seen from the corner of the eye like a mirror light. Yellow and steady while a car is
// there; red and blinking with '왼쪽/오른쪽 차량 주의' once the turn signal is on towards it. Hidden below 30 km/h
// (parking lots, queues) where the sensor fires all the time
static void ui_draw_bsd_mark(const UIState* s, bool right, bool danger) {
    // rounded box with a car seen from the side and a warning triangle under it: yellow and steady while a car is
    // there, red and blinking with '왼쪽/오른쪽 차량 주의' once the turn signal is on towards it
    const float w = 140, h = 170;
    const float x = (s->show_mode == 7) ? (right ? s->fb_w - 30 - 160 - 20 - w : 30 + 340 + 20)
                                        : (right ? s->fb_w - w - 20 : 20);
    const float y = (s->show_mode == 7) ? (s->fb_h - 436.f) : (s->fb_h * 0.60f);
    const bool blink_on = (s->sm->frame % 12) < 7;
    if (danger && !blink_on) return;

    const NVGcolor bg = danger ? nvgRGBA(226, 75, 74, 235) : nvgRGBA(239, 159, 39, 235);
    const NVGcolor fg = danger ? COLOR_WHITE : nvgRGBA(65, 36, 2, 255);
    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, x, y, w, h, 20);
    nvgFillColor(s->vg, bg);
    nvgFill(s->vg);
    nvgStrokeColor(s->vg, COLOR_WHITE);
    nvgStrokeWidth(s->vg, 5);
    nvgStroke(s->vg);

    // car seen from the side (the user's picture), big enough to fill the upper half of the box
    const float cx = x + w / 2, cw = 118, ch = 64, cy = y + 48;  // car width / height, centre of the body
    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, cx - cw / 2, cy - ch * 0.08f, cw, ch * 0.42f, 10);  // lower body
    nvgMoveTo(s->vg, cx - cw * 0.34f, cy - ch * 0.06f);                         // cabin
    nvgLineTo(s->vg, cx - cw * 0.20f, cy - ch * 0.46f);
    nvgLineTo(s->vg, cx + cw * 0.18f, cy - ch * 0.46f);
    nvgLineTo(s->vg, cx + cw * 0.38f, cy - ch * 0.06f);
    nvgClosePath(s->vg);
    nvgFillColor(s->vg, fg);
    nvgFill(s->vg);
    nvgBeginPath(s->vg);                                                        // side windows
    nvgMoveTo(s->vg, cx - cw * 0.27f, cy - ch * 0.10f);
    nvgLineTo(s->vg, cx - cw * 0.16f, cy - ch * 0.37f);
    nvgLineTo(s->vg, cx - cw * 0.02f, cy - ch * 0.37f);
    nvgLineTo(s->vg, cx - cw * 0.02f, cy - ch * 0.10f);
    nvgClosePath(s->vg);
    nvgMoveTo(s->vg, cx + cw * 0.03f, cy - ch * 0.10f);
    nvgLineTo(s->vg, cx + cw * 0.03f, cy - ch * 0.37f);
    nvgLineTo(s->vg, cx + cw * 0.15f, cy - ch * 0.37f);
    nvgLineTo(s->vg, cx + cw * 0.29f, cy - ch * 0.10f);
    nvgClosePath(s->vg);
    nvgFillColor(s->vg, bg);
    nvgFill(s->vg);
    for (int i = -1; i <= 1; i += 2) {                                          // wheels
        nvgBeginPath(s->vg);
        nvgCircle(s->vg, cx + i * cw * 0.29f, cy + ch * 0.36f, ch * 0.17f);
        nvgFillColor(s->vg, fg);
        nvgFill(s->vg);
        nvgStrokeColor(s->vg, bg);
        nvgStrokeWidth(s->vg, 4);
        nvgStroke(s->vg);
        nvgBeginPath(s->vg);
        nvgCircle(s->vg, cx + i * cw * 0.29f, cy + ch * 0.36f, ch * 0.06f);
        nvgFillColor(s->vg, bg);
        nvgFill(s->vg);
    }

    // warning triangle with an exclamation mark under the car, filling the lower half of the box:
    // yellow with a black rim in the yellow box, white with a red rim in the red box
    const float triW = 92, triH = 76, triX = cx - triW / 2, triY = y + h - triH - 8;
    nvgBeginPath(s->vg);
    nvgMoveTo(s->vg, cx, triY);
    nvgLineTo(s->vg, triX + triW, triY + triH);
    nvgLineTo(s->vg, triX, triY + triH);
    nvgClosePath(s->vg);
    nvgLineJoin(s->vg, NVG_ROUND);
    nvgFillColor(s->vg, danger ? COLOR_WHITE : nvgRGBA(250, 184, 0, 255));
    nvgFill(s->vg);
    nvgStrokeColor(s->vg, danger ? nvgRGBA(200, 0, 0, 255) : nvgRGBA(20, 20, 20, 255));
    nvgStrokeWidth(s->vg, danger ? 9 : 6);
    nvgStroke(s->vg);
    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, cx - 5.5f, triY + 24, 11, 30, 5.5f);
    nvgCircle(s->vg, cx, triY + 64, 6);
    nvgFillColor(s->vg, nvgRGBA(20, 20, 20, 255));
    nvgFill(s->vg);

    if (danger) {
        const char* txt = right ? "오른쪽 차량 주의" : "왼쪽 차량 주의";
        const float tw = 360, th = 80;
        const float tx = right ? x - tw - 16 : x + w + 16, ty = y + h / 2 - th / 2;
        nvgBeginPath(s->vg);
        nvgRoundedRect(s->vg, tx, ty, tw, th, 14);
        nvgFillColor(s->vg, nvgRGBA(162, 45, 45, 235));
        nvgFill(s->vg);
        nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        ui_draw_text(s, tx + tw / 2, ty + th / 2 + 2, txt, 48, COLOR_WHITE, BOLD, 0.0f, 0.0f);
    }
}

static void ui_draw_bsd_edge(const UIState* s) {
    if (!s->show_blind_spot || !s->scene.started) return;
    const auto cs = (*s->sm)["carState"].getCarState();
    if (cs.getVEgo() * 3.6f < 30.0f) return;
    if (cs.getLeftBlindspot()) ui_draw_bsd_mark(s, false, cs.getLeftBlinker());
    if (cs.getRightBlindspot()) ui_draw_bsd_mark(s, true, cs.getRightBlinker());
}

void ui_draw(UIState *s, int w, int h) {
  // Update intrinsics matrix after possible wide camera toggle change
  if (s->fb_w != w || s->fb_h != h) {
    ui_resize(s, w, h);
  }
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  //glBlendFunc(GL_DST_COLOR, GL_ZERO);
  nvgBeginFrame(s->vg, s->fb_w, s->fb_h, 1.0f);
  nvgScissor(s->vg, 0, 0, s->fb_w, s->fb_h);
  drawApilot->drawLaneLines(s);
  drawApilot->drawLeadApilot(s);
  drawApilot->drawDebugText(s, s->show_debug);
  drawApilot->drawDeviceState(s, s->show_device_stat);
  ui_draw_gps_info(s);
  ui_draw_bsd_edge(s);

  ui_draw_text_a2(s);

  //ui_draw_vision(s);
  //dashcam(s);
    //param value
  //nvgFontSize(s->vg, 170);
  //nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
  //ui_print(s, s->fb_w/2, s->fb_h/2, "%s", "APILOT");

  ui_draw_alert(s);

  nvgResetScissor(s->vg);
  nvgEndFrame(s->vg);
  glDisable(GL_BLEND);
}
void ui_draw_alert(UIState* s) {
    if (alert.size != cereal::ControlsState::AlertSize::NONE) {
        alert_color = COLOR_ORANGE;
        nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        const float y1 = (s->show_mode == 7) ? (s->fb_h - 350) : (s->fb_h - 300);
        const float y2 = (s->show_mode == 7) ? (s->fb_h - 260) : (s->fb_h - 200);
        const float y3 = (s->show_mode == 7) ? (s->fb_h - 195) : (s->fb_h - 120);
        ui_draw_text(s, s->fb_w / 2, y1, alert.text1.toStdString().c_str(), 100, alert_color, BOLD, 3.0f, 8.0f);
        ui_draw_text(s, s->fb_w / 2, y2, alert.text2.toStdString().c_str(), 70, alert_color, BOLD, 3.0f, 8.0f);
        // optech: a third line on 'take control' alerts saying what happens to openpilot
        const char* line3 = alert.type.contains("immediateDisable") ? "오픈파일럿이 꺼졌습니다"
                          : alert.type.contains("softDisable") ? "오픈파일럿이 곧 꺼집니다" : nullptr;
        if (line3) ui_draw_text(s, s->fb_w / 2, y3, line3, 52, alert_color, BOLD, 3.0f, 8.0f);
    }
}
void ui_update_alert(const Alert& a, const QColor& color) {
    alert_color = nvgRGBA(color.red(), color.green(), color.blue(), color.alpha());
    //printf("r=%d, g=%d, b=%d\n", color.red(), color.green(), color.blue());
    alert = a;
}

void ui_nvg_init(UIState *s) {
  // on EON, we enable MSAA
  s->vg = Hardware::EON() ? nvgCreate(0) : nvgCreate(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
  //s->vg = nvgCreate(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
  assert(s->vg);

  // init fonts
  std::pair<const char *, const char *> fonts[] = {
      {"sans-regular", "../assets/fonts/opensans_regular.ttf"},
      {"sans-semibold", "../assets/fonts/opensans_semibold.ttf"},
      {"sans-bold", "../assets/fonts/opensans_bold.ttf"},
      {"KaiGenGothicKR-Normal", "../assets/addon/font/KaiGenGothicKR-Normal.ttf"},
      {"KaiGenGothicKR-Medium", "../assets/addon/font/KaiGenGothicKR-Medium.ttf"},
      {"KaiGenGothicKR-Bold", "../assets/addon/font/KaiGenGothicKR-Bold.ttf"},
      {"Inter-Bold", "../assets/fonts/Inter-Bold.ttf"},
  };
  for (auto [name, file] : fonts) {
    int font_id = nvgCreateFont(s->vg, name, file);
    assert(font_id >= 0);
  }
  // init images
  std::vector<std::pair<const char *, const char *>> images = {
    {"wheel", "../assets/img_chffr_wheel.png"},
    //{"driver_face", "../assets/img_driver_face.png"},
    //{"speed_bump", "../assets/addon/img/img_speed_bump.png"},
  {"ic_radar", "../assets/images/radar_red.png"},
  {"ic_radar_vision", "../assets/images/radar_vision.png"},
  {"ic_radar_no", "../assets/images/no_radar.png"},
  {"ic_steer_momo", "../assets/images/steer_momo.png"},
  {"ic_steer_red", "../assets/images/steer_red.png"},
  {"ic_steer_green", "../assets/images/steer_green.png"},
  {"ic_steer_yellow", "../assets/images/steer_yellow.png"},
  {"ic_lane_change_l", "../assets/images/lane_change_l.png"},
  {"ic_lane_change_r", "../assets/images/lane_change_r.png"},
  {"ic_lane_change_inhibit", "../assets/images/lane_change_inhibit.png"},
  {"ic_lane_change_steer", "../assets/images/lane_change_steer.png"},
  {"ic_bsd_l", "../assets/images/bsd_l.png"},
  {"ic_bsd_r", "../assets/images/bsd_r.png"},
  {"ic_turn_l", "../assets/images/turn_l.png"},
  {"ic_turn_r", "../assets/images/turn_r.png"},
  {"ic_blinker_l", "../assets/images/blink_l.png"},
  {"ic_blinker_r", "../assets/images/blink_r.png"},
  {"ic_speed_bg", "../assets/images/speed_bg.png"},
  {"ic_traffic_green", "../assets/images/traffic_green.png"},
  {"ic_traffic_red", "../assets/images/traffic_red.png"},
  {"ic_tire", "../assets/images/img_tire.png"},
  {"ic_road_speed", "../assets/images/road_speed.png"},
  {"ic_speed_bump", "../assets/images/speed_bump.png"},
  {"ic_nda", "../assets/images/img_nda.png"},
  {"ic_navi","../assets/images/img_navi.png"},
  {"ic_scc2", "../assets/images/img_scc2.png"},
  {"ic_radartracks", "../assets/images/img_radartracks.png"},
  {"ic_apm", "../assets/images/img_apm.png"},
  {"ic_apn", "../assets/images/img_apn.png"},
  {"ic_hda", "../assets/images/img_hda.png"},

  };
  for (auto [name, file] : images) {
    s->images[name] = nvgCreateImage(s->vg, file, 1);
    assert(s->images[name] != 0);
  }
  drawApilot = new DrawApilot();
}
void ui_resize(UIState *s, int width, int height) {
  s->fb_w = width;
  s->fb_h = height;
#if 0
  auto intrinsic_matrix = s->wide_camera ? ecam_intrinsic_matrix : fcam_intrinsic_matrix;
  float zoom = ZOOM / intrinsic_matrix.v[0];
  if (s->wide_camera) {
    zoom *= 0.5;
  }

  // Apply transformation such that video pixel coordinates match video
  // 1) Put (0, 0) in the middle of the video
  // 2) Apply same scaling as video
  // 3) Put (0, 0) in top left corner of video
  s->car_space_transform.reset();
  s->car_space_transform.translate(width / 2, height / 2 + y_offset)
      .scale(zoom, zoom)
      .translate(-intrinsic_matrix.v[2], -intrinsic_matrix.v[5]);
#endif
}
