// ============================================================================
// 任务10 v4 · AnalyzeFrame 实现（纯算法、零板依赖、可单独编译/单测）
//
// 输入：一帧 GC0308 YUV422（每像素 2 字节，布局 [Y0][Cb][Y1][Cr]，Cb/Cr 每 2 像素一组）
// 输出：肤色占比 / 重心偏移 / 包围盒长宽比 / 三档命中标志（FrameStat）
// ============================================================================
#include "person_aim.h"

FrameStat AnalyzeFrame(const uint8_t* yuyv, size_t len, uint16_t w, uint16_t h,
                       const AimTuning& t) {
    FrameStat o;
    o.w = w;
    o.h = h;
    if (!yuyv || w == 0 || h == 0 || len < 4) return o;   // 拿不到帧 → 空（薄壳按没找到）

    int minx = 1 << 20, miny = 1 << 20;                    // 肤色包围盒
    int maxx = -1, maxy = -1;
    long sx = 0, sy = 0;                                   // 肤色坐标累加 → 重心

    for (int row = 0; row < (int)h; row += t.sample_step) {
        for (int col = 0; col < (int)w; col += t.sample_step) {
            size_t yofs = ((size_t)row * (size_t)w + (size_t)col) * 2;   // 像素的 Y0
            if (yofs + 3 >= len) continue;
            uint8_t Y  = yuyv[yofs];        // [Y0]
            uint8_t Cb = yuyv[yofs + 1];    // [Cb]
            uint8_t Cr = yuyv[yofs + 3];    // [Cr]（跳过 [Y1]，同一组）
            o.samples++;
            if (Y >= t.y_min && Y <= t.y_max &&
                Cb >= t.cb_min && Cb <= t.cb_max &&
                Cr >= t.cr_min && Cr <= t.cr_max) {
                o.skin++;
                sx += col; sy += row;
                if (col < minx) minx = col; if (col > maxx) maxx = col;
                if (row < miny) miny = row; if (row > maxy) maxy = row;
            }
        }
    }

    if (o.samples == 0) return o;
    o.ratio = (float)o.skin / (float)o.samples;

    if (o.skin > 0) {
        o.bbox_w = maxx - minx + 1;
        o.bbox_h = maxy - miny + 1;
        float cx = (float)sx / (float)o.skin;
        float cy = (float)sy / (float)o.skin;
        o.dx = (w > 0) ? (cx / ((float)w / 2.0f) - 1.0f) : 0.0f;
        o.dy = (h > 0) ? (cy / ((float)h / 2.0f) - 1.0f) : 0.0f;
        o.bbox_ratio = (o.bbox_h > 0) ? ((float)o.bbox_w / (float)o.bbox_h) : 9.0f;
    }

    bool bbox_ok = (o.bbox_ratio >= t.bbox_ratio_min && o.bbox_ratio <= t.bbox_ratio_max);
    o.hit        = o.skin > 0 && o.ratio >= t.ratio_min && o.ratio <= t.ratio_max && bbox_ok;
    o.hit_strong = o.skin > 0 && o.ratio >= t.strong_ratio && o.ratio <= t.ratio_max && bbox_ok;
    o.hit_weak   = o.hit && !o.hit_strong;   // [ratio_min, strong_ratio)
    return o;
}
