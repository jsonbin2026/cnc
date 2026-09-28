package com.example.gk;

import android.app.Activity;
import android.os.Bundle;
import android.widget.ImageView;
import android.widget.TextView;

public class FeatureActivity extends Activity {

    public static final String EXTRA_ID = "feature_id";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_feature);

        int id = getIntent().getIntExtra(EXTRA_ID, -1);
        Catalog catalog = Catalog.get();
        Feature feature = catalog.byId(id);

        findViewById(R.id.back).setOnClickListener(v -> finish());

        if (feature == null) {
            ((TextView) findViewById(R.id.title)).setText("未找到");
            return;
        }

        Section section = null;
        for (Section s : catalog.sections()) {
            if (s.contains(feature.id)) {
                section = s;
                break;
            }
        }

        ((TextView) findViewById(R.id.title)).setText(feature.name);
        ((TextView) findViewById(R.id.name)).setText(feature.name);

        TextView status = findViewById(R.id.status);
        status.setText(feature.status);
        status.setTextColor(MainActivity.statusColor(feature));

        if (section != null) {
            ((ImageView) findViewById(R.id.icon)).setImageResource(section.iconRes);
        }

        ((TextView) findViewById(R.id.row_id)).setText("编号：  #" + feature.id);
        ((TextView) findViewById(R.id.row_domain)).setText("领域：  " + feature.domain);
        ((TextView) findViewById(R.id.row_section)).setText("模块：  "
                + (section != null ? section.index + ". " + section.name : "—"));
    }
}
