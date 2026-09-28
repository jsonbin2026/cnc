package com.example.gk;

import java.util.ArrayList;
import java.util.List;

/** A top-level module (one of the 47 sections from the feature document). */
public final class Section {
    public final int index;      // 1..47
    public final String name;    // display name
    public final int firstId;    // inclusive
    public final int lastId;     // inclusive
    public final int iconRes;

    public Section(int index, String name, int firstId, int lastId, int iconRes) {
        this.index = index;
        this.name = name;
        this.firstId = firstId;
        this.lastId = lastId;
        this.iconRes = iconRes;
    }

    public boolean contains(int id) {
        return id >= firstId && id <= lastId;
    }

    /** The 47 modules, matching 功能大全.txt. */
    public static List<Section> all() {
        List<Section> s = new ArrayList<>();
        s.add(new Section(1, "基础功能", 1, 30, R.drawable.ic_mod_machine));
        s.add(new Section(2, "G 代码功能", 31, 82, R.drawable.ic_mod_gcode));
        s.add(new Section(3, "M 代码功能", 83, 101, R.drawable.ic_mod_mcode));
        s.add(new Section(4, "宏程序功能", 102, 114, R.drawable.ic_mod_macro));
        s.add(new Section(5, "运动控制", 115, 138, R.drawable.ic_mod_motion));
        s.add(new Section(6, "材料去除与工艺", 139, 170, R.drawable.ic_mod_material));
        s.add(new Section(7, "碰撞检测", 171, 178, R.drawable.ic_mod_collision));
        s.add(new Section(8, "机床类型", 179, 208, R.drawable.ic_mod_machine));
        s.add(new Section(9, "CNC 系统", 209, 231, R.drawable.ic_mod_cnc));
        s.add(new Section(10, "机床动作", 232, 259, R.drawable.ic_mod_action));
        s.add(new Section(11, "测量与质检", 260, 279, R.drawable.ic_mod_measure));
        s.add(new Section(12, "教学系统", 280, 318, R.drawable.ic_mod_teach));
        s.add(new Section(13, "认知科学", 319, 332, R.drawable.ic_mod_brain));
        s.add(new Section(14, "故障与异常", 333, 358, R.drawable.ic_mod_fault));
        s.add(new Section(15, "操作界面", 359, 380, R.drawable.ic_mod_ui));
        s.add(new Section(16, "操作面板", 381, 423, R.drawable.ic_mod_panel));
        s.add(new Section(17, "程序编辑", 424, 439, R.drawable.ic_mod_editor));
        s.add(new Section(18, "交互", 440, 453, R.drawable.ic_mod_chat));
        s.add(new Section(19, "数据与文件", 454, 478, R.drawable.ic_mod_data));
        s.add(new Section(20, "可视化分析", 479, 495, R.drawable.ic_mod_chart));
        s.add(new Section(21, "数字化与工业接口", 496, 515, R.drawable.ic_mod_iot));
        s.add(new Section(22, "AI 与数据科学", 516, 540, R.drawable.ic_mod_ai));
        s.add(new Section(23, "协作与课堂", 541, 560, R.drawable.ic_mod_people));
        s.add(new Section(24, "内容生态", 561, 580, R.drawable.ic_mod_eco));
        s.add(new Section(25, "物理仿真深水区", 581, 600, R.drawable.ic_mod_physics));
        s.add(new Section(26, "材料与工艺库", 601, 621, R.drawable.ic_mod_flask));
        s.add(new Section(27, "制造全流程", 622, 640, R.drawable.ic_mod_flow));
        s.add(new Section(28, "能源与成本", 641, 655, R.drawable.ic_mod_cost));
        s.add(new Section(29, "维护与保养", 656, 675, R.drawable.ic_mod_wrench));
        s.add(new Section(30, "安全与规范", 676, 700, R.drawable.ic_mod_shield));
        s.add(new Section(31, "不确定性与极端场景", 701, 730, R.drawable.ic_mod_dice));
        s.add(new Section(32, "感官扩展", 731, 754, R.drawable.ic_mod_eye));
        s.add(new Section(33, "跨媒介表达", 755, 768, R.drawable.ic_mod_media));
        s.add(new Section(34, "逆向与叙事", 769, 788, R.drawable.ic_mod_history));
        s.add(new Section(35, "跨尺度与跨时间", 789, 805, R.drawable.ic_mod_scale));
        s.add(new Section(36, "多物理场", 806, 819, R.drawable.ic_mod_waves));
        s.add(new Section(37, "CAD/CAM 集成", 820, 846, R.drawable.ic_mod_cube));
        s.add(new Section(38, "平台与工程化", 847, 874, R.drawable.ic_mod_cloud));
        s.add(new Section(39, "商业模式", 875, 893, R.drawable.ic_mod_business));
        s.add(new Section(40, "无障碍与特殊需求", 894, 903, R.drawable.ic_mod_a11y));
        s.add(new Section(41, "数据科学层", 904, 918, R.drawable.ic_mod_sigma));
        s.add(new Section(42, "硬件在环 HIL", 919, 930, R.drawable.ic_mod_hil));
        s.add(new Section(43, "学术研究接口", 931, 940, R.drawable.ic_mod_academic));
        s.add(new Section(44, "安全合规", 941, 950, R.drawable.ic_mod_compliance));
        s.add(new Section(45, "其他高级功能", 951, 970, R.drawable.ic_mod_star));
        s.add(new Section(46, "更接近真实机床", 971, 1475, R.drawable.ic_mod_spark));
        s.add(new Section(47, "更多真实机床特性", 1476, 1525, R.drawable.ic_mod_spark2));
        return s;
    }
}
