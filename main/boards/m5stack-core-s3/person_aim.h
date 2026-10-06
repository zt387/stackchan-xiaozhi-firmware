#pragma once
// ============================================================================
// 任务10 v4 · 拍前「有没有人样」的纯算法（零依赖、可单独编译）
//
// 只放纯算法：不 include 任何板类（FaceTracker / StackChanServo 都定义在
// m5stack_core_s3.cc 里）、不 include ESP、不打日志。
// 板内薄壳（m5stack_core_s3.cc）负责：PeekFrame 取帧 → 调 AnalyzeFrame →
// 按 FrameStat 三档行动（挪正/直接拍/扫）→ 打 [PersonAim] 日志。
//
// 采样布局（GC0308 YUV422 / 每像素 2 字节）：[Y0][Cb][Y1][Cr]，Cb/Cr 每 2 像素一组
// ============================================================================
#include <cstdint>
#include <cstring>
#include <cmath>

// ---- 所有阈值都集中在这，觉得太敏感/太迟钝改这里就行（v4 第四节）----
struct AimTuning {
    // —— 肤色阈值（YCbCr 经典范围）——
    int    y_min = 40,  y_max = 240;  // 亮度：别太窄，光线偏暗/偏黄也要能拍到
    int    cb_min = 77, cb_max = 127; // 蓝色差
    int    cr_min = 133, cr_max = 173;// 红色差

    // —— 「像个人」的肤色占比上下限 ——
    float  ratio_min = 0.015f;        // 低于 = 没命中（去扫）
    float  ratio_max = 0.60f;         // 高于 = 整屏暖色 / 明显不是人
    float  strong_ratio = 0.04f;      // ≥4% 强命中（才值得挪正）；1.5%~4% 弱命中直接拍

    // —— 包围盒长宽比（宽/高），排除「一条细线」「整屏暖色」——
    float  bbox_ratio_min = 0.4f, bbox_ratio_max = 2.2f;

    // —— 挪正：死区 ±12%，最多 4 次微调，超过也拍（别扫个没完）——
    float  deadzone = 0.12f;
    int    nudge_times = 4;

    // —— 慢扫：±30°，每轮 6 步，每步停 150ms，最多 2 轮 ——
    int    scan_deg = 30;
    int    scan_steps = 6;
    int    step_delay_ms = 150;
    int    scan_step_ms = 250;        // 扫描每步停留（明显、看得见）
    int    max_rounds = 2;

    // —— 采样步距：每 4 像素取 1 组（320x240 → 80x60 = 4800 点，几毫秒）——
    int    sample_step = 4;
};

// 一帧的统计结果
struct FrameStat {
    uint16_t w = 0, h = 0;
    int    samples = 0;              // 采样点数
    int    skin = 0;                 // 肤色点数
    float  ratio = 0.0f;             // 肤色占比 = skin / samples
    float  dx = 0.0f, dy = 0.0f;     // 重心偏移，归一化 ±1（-1 左/上，+1 右/下）
    int    bbox_w = 0, bbox_h = 0;   // 肤色包围盒（像素）
    float  bbox_ratio = 0.0f;        // 包围盒宽/高
    float  ms = 0.0f;                // 本次耗时（毫秒），薄壳填
    bool   hit = false;              // 「像个人」：占比与 bbox 都在区间
    bool   hit_strong = false;       // 占比 ≥ strong_ratio 且 bbox 合理 → 值得挪正
    bool   hit_weak = false;         // [ratio_min, strong_ratio) 且 bbox 合理 → 直接拍
};

// 分析一帧 YUV422 原始数据（不依赖相机/板，可单测）
FrameStat AnalyzeFrame(const uint8_t* yuyv, size_t len, uint16_t w, uint16_t h,
                       const AimTuning& t);
