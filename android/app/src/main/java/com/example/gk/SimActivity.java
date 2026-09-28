package com.example.gk;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class SimActivity extends Activity {

    static {
        System.loadLibrary("native-lib");
    }

    public static native void nativeSimInit();
    public static native void nativeSimLoadDemo();
    public static native boolean nativeSimStep();
    public static native double[] nativeSimToolPos();
    public static native int[] nativeSimVoxels();
    public static native int nativeSimRemoved();
    public static native int nativeSimBlocksDone();
    public static native boolean nativeSimFinished();

    private GLSurfaceView glView;
    private SimRenderer renderer;
    private TextView status;
    private Button runButton;
    private final Handler handler = new Handler(Looper.getMainLooper());

    private boolean running = false;
    private float lastX, lastY;
    private boolean dragging;
    private long lastVoxelUpdate = 0;

    private final Runnable loop = new Runnable() {
        @Override
        public void run() {
            if (!running) {
                return;
            }
            boolean more = nativeSimStep();
            double[] p = nativeSimToolPos();
            renderer.setToolPos(p[0], p[1], p[2]);

            long now = System.currentTimeMillis();
            if (now - lastVoxelUpdate > 300) {
                renderer.updateVoxels(nativeSimVoxels());
                lastVoxelUpdate = now;
            }
            status.setText("已执行 " + nativeSimBlocksDone() + " 段 · 去除 "
                    + nativeSimRemoved() + " 个体素");

            if (!more || nativeSimFinished()) {
                running = false;
                runButton.setText("运行");
                renderer.updateVoxels(nativeSimVoxels());
                status.setText("完成 · 共去除 " + nativeSimRemoved() + " 个体素");
                return;
            }
            handler.postDelayed(this, 60);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sim);

        nativeSimInit();
        nativeSimLoadDemo();

        renderer = new SimRenderer();
        renderer.updateVoxels(nativeSimVoxels());

        glView = findViewById(R.id.gl);
        glView.setEGLContextClientVersion(3);
        glView.setRenderer(renderer);
        glView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        glView.setOnTouchListener((v, e) -> {
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    lastX = e.getX(); lastY = e.getY(); dragging = true;
                    return true;
                case MotionEvent.ACTION_MOVE:
                    if (dragging) {
                        renderer.orbit((e.getX() - lastX) * 0.4f,
                                -(e.getY() - lastY) * 0.4f);
                        lastX = e.getX(); lastY = e.getY();
                        glView.requestRender();
                    }
                    return true;
                case MotionEvent.ACTION_POINTER_DOWN:
                    // pinch handled loosely: treat as zoom out
                    return true;
                case MotionEvent.ACTION_UP:
                    dragging = false;
                    return true;
                default:
                    return false;
            }
        });

        status = findViewById(R.id.sim_status);
        runButton = findViewById(R.id.sim_run);
        runButton.setOnClickListener(v -> toggleRun());

        findViewById(R.id.sim_reset).setOnClickListener(v -> reset());
        findViewById(R.id.sim_back).setOnClickListener(v -> finish());
    }

    private void toggleRun() {
        if (running) {
            running = false;
            runButton.setText("继续");
            handler.removeCallbacks(loop);
        } else {
            running = true;
            runButton.setText("暂停");
            handler.post(loop);
        }
    }

    private void reset() {
        running = false;
        runButton.setText("运行");
        handler.removeCallbacks(loop);
        nativeSimInit();
        nativeSimLoadDemo();
        renderer.setToolPos(0, 0, 0);
        renderer.updateVoxels(nativeSimVoxels());
        glView.requestRender();
        status.setText("就绪");
    }

    @Override
    protected void onPause() {
        super.onPause();
        running = false;
        handler.removeCallbacks(loop);
        glView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        glView.onResume();
    }
}
