#include "gk/gk_codes.h"

#include <stddef.h>

static const gk_code_def k_gcodes[] = {
    { 0,  0, "G00 快速定位" },
    { 1,  0, "G01 直线插补" },
    { 2,  0, "G02 圆弧插补 CW" },
    { 3,  0, "G03 圆弧插补 CCW" },
    { 4,  0, "G04 暂停" },
    { 10, 0, "G10 可编程数据输入" },
    { 17, 0, "G17 XY 平面选择" },
    { 18, 0, "G18 ZX 平面选择" },
    { 19, 0, "G19 YZ 平面选择" },
    { 20, 0, "G20 英寸单位" },
    { 21, 0, "G21 公制单位" },
    { 28, 0, "G28 回参考点" },
    { 30, 0, "G30 第二参考点" },
    { 31, 0, "G31 跳转功能" },
    { 32, 0, "G32 螺纹切削" },
    { 40, 0, "G40 取消刀补" },
    { 41, 0, "G41 左刀补" },
    { 42, 0, "G42 右刀补" },
    { 43, 0, "G43 刀长正补偿" },
    { 44, 0, "G44 刀长负补偿" },
    { 49, 0, "G49 取消刀长补偿" },
    { 50, 0, "G50 主轴最高转速限制" },
    { 51, 0, "G51 缩放" },
    { 52, 0, "G52 局部坐标系" },
    { 53, 0, "G53 机床坐标系" },
    { 54, 0, "G54 工件坐标系 1" },
    { 55, 0, "G55 工件坐标系 2" },
    { 56, 0, "G56 工件坐标系 3" },
    { 57, 0, "G57 工件坐标系 4" },
    { 58, 0, "G58 工件坐标系 5" },
    { 59, 0, "G59 工件坐标系 6" },
    { 65, 0, "G65 宏程序调用" },
    { 68, 0, "G68 坐标旋转" },
    { 69, 0, "G69 取消坐标旋转" },
    { 73, 0, "G73 高速深孔钻" },
    { 74, 0, "G74 左旋攻丝" },
    { 76, 0, "G76 精镗" },
    { 80, 0, "G80 取消固定循环" },
    { 81, 0, "G81 钻孔循环" },
    { 82, 0, "G82 钻孔循环（带暂停）" },
    { 83, 0, "G83 深孔钻循环" },
    { 84, 0, "G84 攻丝循环" },
    { 85, 0, "G85 镗孔循环" },
    { 86, 0, "G86 镗孔循环" },
    { 87, 0, "G87 反镗循环" },
    { 88, 0, "G88 镗孔循环" },
    { 89, 0, "G89 镗孔循环" },
    { 90, 0, "G90 绝对值编程" },
    { 91, 0, "G91 增量值编程" },
    { 92, 0, "G92 坐标设定" },
    { 94, 0, "G94 每分钟进给" },
    { 95, 0, "G95 每转进给" },
    { 96, 0, "G96 恒线速" },
    { 97, 0, "G97 恒转速" },
    { 98, 0, "G98 返回初始点" },
    { 99, 0, "G99 返回 R 点" },
    { 6,  2, "G06.2 NURBS 插补" },
};

static const gk_code_def k_mcodes[] = {
    { 0,  0, "M00 程序停止" },
    { 1,  0, "M01 可选停止" },
    { 2,  0, "M02 程序结束" },
    { 3,  0, "M03 主轴正转" },
    { 4,  0, "M04 主轴反转" },
    { 5,  0, "M05 主轴停止" },
    { 6,  0, "M06 自动换刀" },
    { 7,  0, "M07 雾状冷却" },
    { 8,  0, "M08 液体冷却" },
    { 9,  0, "M09 冷却停止" },
    { 10, 0, "M10 夹紧" },
    { 11, 0, "M11 松开" },
    { 13, 0, "M13 主轴正转+冷却" },
    { 14, 0, "M14 主轴反转+冷却" },
    { 19, 0, "M19 主轴定向" },
    { 29, 0, "M29 刚性攻丝" },
    { 30, 0, "M30 程序结束复位" },
    { 98, 0, "M98 子程序调用" },
    { 99, 0, "M99 子程序返回" },
};

const gk_code_def *gk_gcode_table(size_t *count)
{
    if (count != NULL) {
        *count = sizeof(k_gcodes) / sizeof(k_gcodes[0]);
    }
    return k_gcodes;
}

const gk_code_def *gk_mcode_table(size_t *count)
{
    if (count != NULL) {
        *count = sizeof(k_mcodes) / sizeof(k_mcodes[0]);
    }
    return k_mcodes;
}

const char *gk_gcode_lookup(int code, int sub)
{
    size_t i;
    size_t n = sizeof(k_gcodes) / sizeof(k_gcodes[0]);
    for (i = 0; i < n; ++i) {
        if (k_gcodes[i].code == code && k_gcodes[i].sub == sub) {
            return k_gcodes[i].name;
        }
    }
    return NULL;
}

const char *gk_mcode_lookup(int code)
{
    size_t i;
    size_t n = sizeof(k_mcodes) / sizeof(k_mcodes[0]);
    for (i = 0; i < n; ++i) {
        if (k_mcodes[i].code == code) {
            return k_mcodes[i].name;
        }
    }
    return NULL;
}
