package com.example.gk;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import java.util.List;

public class SectionActivity extends Activity {

    public static final String EXTRA_INDEX = "section_index";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_section);

        int index = getIntent().getIntExtra(EXTRA_INDEX, 1);
        Catalog catalog = Catalog.get();
        Section section = catalog.sections().get(index - 1);
        List<Feature> features = catalog.featuresOf(section);

        ((ImageView) findViewById(R.id.title_icon)).setImageResource(section.iconRes);
        ((TextView) findViewById(R.id.title)).setText(section.name);
        ((TextView) findViewById(R.id.subtitle)).setText(
                "#" + section.index + " · " + features.size() + " 项");

        findViewById(R.id.back).setOnClickListener(v -> finish());

        ListView list = findViewById(R.id.list);
        list.setAdapter(new FeatureAdapter(this, features));
        list.setOnItemClickListener((parent, view, position, id) -> {
            Feature f = features.get(position);
            Intent i = new Intent(SectionActivity.this, FeatureActivity.class);
            i.putExtra(FeatureActivity.EXTRA_ID, f.id);
            startActivity(i);
        });
    }

    private static final class FeatureAdapter extends BaseAdapter {
        private final LayoutInflater inflater;
        private final List<Feature> features;

        FeatureAdapter(Context ctx, List<Feature> features) {
            this.inflater = LayoutInflater.from(ctx);
            this.features = features;
        }

        @Override
        public int getCount() {
            return features.size();
        }

        @Override
        public Object getItem(int position) {
            return features.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View v = convertView;
            if (v == null) {
                v = inflater.inflate(R.layout.item_feature, parent, false);
            }
            Feature f = features.get(position);
            ((TextView) v.findViewById(R.id.id)).setText(String.valueOf(f.id));
            ((TextView) v.findViewById(R.id.name)).setText(f.name);
            TextView status = v.findViewById(R.id.status);
            status.setText(f.status);
            status.setTextColor(MainActivity.statusColor(f));
            return v;
        }
    }
}
