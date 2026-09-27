package com.example.gk;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends Activity {

    static {
        System.loadLibrary("native-lib");
    }

    public native String nativeCatalogDump();
    public native int nativeFeatureCount();
    public native int nativeCountByStatus(int status);
    public native int nativeValidate();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView summary = findViewById(R.id.summary);
        TextView catalog = findViewById(R.id.catalog);

        int total = nativeFeatureCount();
        int verified = nativeCountByStatus(4);
        int implemented = nativeCountByStatus(3);
        int errors = nativeValidate();

        summary.setText(
                "Features: " + total
                        + "\nVerified: " + verified
                        + "\nImplemented: " + implemented
                        + "\nValidation errors: " + errors);

        catalog.setText(nativeCatalogDump());
    }
}
