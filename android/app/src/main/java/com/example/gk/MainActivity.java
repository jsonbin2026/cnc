package com.example.gk;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import java.util.List;

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

        TextView subtitle = findViewById(R.id.subtitle);
        subtitle.setText("共 " + catalog.total() + " 项功能 · "
                + catalog.sections().size() + " 个模块");

        ListView list = findViewById(R.id.list);
        list.setAdapter(new SectionAdapter(this, catalog.sections()));
        list.setOnItemClickListener((parent, view, position, id) -> {
            Section s = catalog.sections().get(position);
            Intent i = new Intent(MainActivity.this, SectionActivity.class);
            i.putExtra(SectionActivity.EXTRA_INDEX, s.index);
            startActivity(i);
        });
    }

    private static final class SectionAdapter extends BaseAdapter {
        private final LayoutInflater inflater;
        private final List<Section> sections;

        SectionAdapter(Context ctx, List<Section> sections) {
            this.inflater = LayoutInflater.from(ctx);
            this.sections = sections;
        }

        @Override
        public int getCount() {
            return sections.size();
        }

        @Override
        public Object getItem(int position) {
            return sections.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View v = convertView;
            if (v == null) {
                v = inflater.inflate(R.layout.item_section, parent, false);
            }
            Section s = sections.get(position);
            ((ImageView) v.findViewById(R.id.icon)).setImageResource(s.iconRes);
            ((TextView) v.findViewById(R.id.module_name)).setText(s.name);
            ((TextView) v.findViewById(R.id.module_code)).setText(
                    "#" + s.index + "   " + s.firstId + "–" + s.lastId);
            ((TextView) v.findViewById(R.id.count)).setText(
                    (s.lastId - s.firstId + 1) + " 项");
            return v;
        }
    }

    /** Color for a feature status label. */
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
