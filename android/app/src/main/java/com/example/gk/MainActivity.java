package com.example.gk;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("native-lib");
    }

    public static native String nativeCatalogDump();
    public static native int nativeFeatureCount();
    public static native int nativeCountByStatus(int status);
    public static native int nativeValidate();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Catalog catalog = Catalog.get();
        ((TextView) findViewById(R.id.subtitle)).setText(
                "共 " + catalog.total() + " 项功能 · " + catalog.sections().size() + " 个模块");

        // 代码
        setupModule(R.id.head_code, R.id.body_code, R.id.arrow_code, new Item[] {
            new Item("G 代码加工模拟", () -> startActivity(new Intent(this, SimActivity.class))),
            new Item("G 代码功能清单", () -> openSection(2)),
            new Item("M 代码功能清单", () -> openSection(3)),
            new Item("宏程序功能清单", () -> openSection(4)),
        });

        // 刀具
        setupModule(R.id.head_tool, R.id.body_tool, R.id.arrow_tool, new Item[] {
            new Item("刀具库与刀具系统", () -> openSection(29)),
            new Item("刀具补偿", () -> openSection(2)),
            new Item("刀具在模拟中显示", () -> startActivity(new Intent(this, SimActivity.class))),
        });

        // 材料
        setupModule(R.id.head_material, R.id.body_material, R.id.arrow_material, new Item[] {
            new Item("材料去除与工艺", () -> openSection(6)),
            new Item("材料与工艺库", () -> openSection(26)),
            new Item("体素毛坯切削演示", () -> startActivity(new Intent(this, SimActivity.class))),
        });

        // 功能
        setupModule(R.id.head_features, R.id.body_features, R.id.arrow_features, new Item[] {
            new Item("全部 47 个模块", () -> startActivity(new Intent(this, CatalogActivity.class))),
        });
    }

    private void openSection(int index) {
        Intent i = new Intent(this, SectionActivity.class);
        i.putExtra(SectionActivity.EXTRA_INDEX, index);
        startActivity(i);
    }

    private void setupModule(int headId, int bodyId, int arrowId, Item[] items) {
        LinearLayout body = findViewById(bodyId);
        TextView arrow = findViewById(arrowId);
        LayoutInflater inflater = LayoutInflater.from(this);
        for (Item it : items) {
            TextView row = new TextView(this);
            row.setText("·  " + it.label);
            row.setTextColor(Color.parseColor("#E6EDF3"));
            row.setTextSize(14f);
            row.setPadding(46, 26, 26, 26);
            row.setBackgroundResource(R.drawable.row_bg);
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    LinearLayout.LayoutParams.WRAP_CONTENT);
            lp.bottomMargin = 4;
            row.setLayoutParams(lp);
            row.setOnClickListener(v -> it.action.run());
            body.addView(row);
        }
        findViewById(headId).setOnClickListener(v -> {
            boolean opening = body.getVisibility() != View.VISIBLE;
            body.setVisibility(opening ? View.VISIBLE : View.GONE);
            arrow.setText(opening ? "▾" : "▸");
        });
    }

    private static final class Item {
        final String label;
        final Runnable action;
        Item(String label, Runnable action) {
            this.label = label;
            this.action = action;
        }
    }

    static int statusColor(Feature f) {
        if (f.isVerified()) {
            return Color.parseColor("#3FB950");
        }
        if (f.isImplemented()) {
            return Color.parseColor("#58A6FF");
        }
        if ("PARTIAL".equals(f.status)) {
            return Color.parseColor("#D29922");
        }
        return Color.parseColor("#8B98A5");
    }
}
