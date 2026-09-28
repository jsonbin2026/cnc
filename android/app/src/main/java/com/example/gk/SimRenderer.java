package com.example.gk;

import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** Renders the CNC simulator: stock voxels, machine frame and the tool. */
final class SimRenderer implements GLSurfaceView.Renderer {

    private static final int NX = 64;
    private static final int NY = 64;
    private static final int NZ = 32;
    private static final double CELL = 1.0;
    private static final double OX = -32.0;
    private static final double OY = -32.0;
    private static final double OZ = -32.0;

    private final float[] mvp = new float[16];
    private final float[] proj = new float[16];
    private final float[] view = new float[16];

    private int solidProgram;
    private int lineProgram;
    private int solidMvp;
    private int solidPos;
    private int solidColor;
    private int lineMvp;
    private int lineColor;
    private int linePos;

    private int solidVbo;
    private int solidVboColor;
    private int lineVbo;
    private int frameVbo;
    private int gridVbo;
    private int gridCount;
    private int frameCount;

    private FloatBuffer solidVerts;
    private FloatBuffer solidColors;
    private int solidCount;

    // camera orbit
    private float yaw = 35f;
    private float pitch = 28f;
    private float dist = 150f;

    private volatile double toolX, toolY, toolZ;

    SimRenderer() {
        solidVerts = allocFloat(NX * NY * NZ * 3);
        solidColors = allocFloat(NX * NY * NZ * 3);
    }

    private static FloatBuffer allocFloat(int n) {
        ByteBuffer bb = ByteBuffer.allocateDirect(n * 4);
        bb.order(ByteOrder.nativeOrder());
        return bb.asFloatBuffer();
    }

    void setToolPos(double x, double y, double z) {
        toolX = x;
        toolY = y;
        toolZ = z;
    }

    void orbit(float dx, float dy) {
        yaw += dx;
        pitch += dy;
        if (pitch > 85f) pitch = 85f;
        if (pitch < 5f) pitch = 5f;
    }

    void zoom(float factor) {
        dist *= factor;
        if (dist < 60f) dist = 60f;
        if (dist > 400f) dist = 400f;
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES30.glClearColor(0.055f, 0.067f, 0.086f, 1f);
        GLES30.glEnable(GLES30.GL_DEPTH_TEST);

        solidProgram = buildProgram(Shaders.VERTEX, Shaders.FRAGMENT);
        solidMvp = GLES30.glGetUniformLocation(solidProgram, "uMvp");
        solidPos = GLES30.glGetAttribLocation(solidProgram, "aPos");
        solidColor = GLES30.glGetAttribLocation(solidProgram, "aColor");

        lineProgram = buildProgram(Shaders.LINE_VERTEX, Shaders.LINE_FRAGMENT);
        lineMvp = GLES30.glGetUniformLocation(lineProgram, "uMvp");
        lineColor = GLES30.glGetUniformLocation(lineProgram, "uColor");
        linePos = GLES30.glGetAttribLocation(lineProgram, "aPos");

        int[] bufs = new int[4];
        GLES30.glGenBuffers(4, bufs, 0);
        solidVbo = bufs[0];
        solidVboColor = bufs[1];
        frameVbo = bufs[2];
        gridVbo = bufs[3];
        lineVbo = 0;
        buildFrame();
        buildGrid();
    }

    private void buildFrame() {
        // Machine envelope wireframe box: 70 x 70 x 40 around origin
        float x0 = -35, x1 = 35, y0 = -35, y1 = 35, z0 = -32, z1 = 8;
        float[] v = {
            x0,y0,z0, x1,y0,z0,  x1,y0,z0, x1,y1,z0,
            x1,y1,z0, x0,y1,z0,  x0,y1,z0, x0,y0,z0,
            x0,y0,z1, x1,y0,z1,  x1,y0,z1, x1,y1,z1,
            x1,y1,z1, x0,y1,z1,  x0,y1,z1, x0,y0,z1,
            x0,y0,z0, x0,y0,z1,  x1,y0,z0, x1,y0,z1,
            x1,y1,z0, x1,y1,z1,  x0,y1,z0, x0,y1,z1
        };
        frameCount = v.length / 3;
        FloatBuffer fb = toBuffer(v);
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, frameVbo);
        GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, v.length * 4, fb, GLES30.GL_STATIC_DRAW);
    }

    private void buildGrid() {
        java.util.ArrayList<Float> v = new java.util.ArrayList<>();
        for (int i = -32; i <= 32; i += 8) {
            v.add((float) i); v.add(-32f); v.add(8f);
            v.add((float) i); v.add(32f); v.add(8f);
            v.add(-32f); v.add((float) i); v.add(8f);
            v.add(32f); v.add((float) i); v.add(8f);
        }
        float[] a = new float[v.size()];
        for (int i = 0; i < a.length; i++) a[i] = v.get(i);
        gridCount = a.length / 3;
        FloatBuffer fb = toBuffer(a);
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, gridVbo);
        GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, a.length * 4, fb, GLES30.GL_STATIC_DRAW);
    }

    private static FloatBuffer toBuffer(float[] a) {
        FloatBuffer fb = ByteBuffer.allocateDirect(a.length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
        fb.put(a).position(0);
        return fb;
    }

    private static int buildProgram(String vs, String fs) {
        int v = compile(GLES30.GL_VERTEX_SHADER, vs);
        int f = compile(GLES30.GL_FRAGMENT_SHADER, fs);
        int p = GLES30.glCreateProgram();
        GLES30.glAttachShader(p, v);
        GLES30.glAttachShader(p, f);
        GLES30.glLinkProgram(p);
        return p;
    }

    private static int compile(int type, String src) {
        int s = GLES30.glCreateShader(type);
        GLES30.glShaderSource(s, src);
        GLES30.glCompileShader(s);
        return s;
    }

    /** Rebuild the stock mesh from a native voxel occupancy array. */
    void updateVoxels(int[] cells) {
        solidVerts.clear();
        solidColors.clear();
        int n = 0;
        int idx = 0;
        for (int ix = 0; ix < NX; ix++) {
            for (int iy = 0; iy < NY; iy++) {
                for (int iz = 0; iz < NZ; iz++, idx++) {
                    if (cells[idx] == 0) continue;
                    float wx = (float) (OX + (ix + 0.5) * CELL);
                    float wy = (float) (OY + (iy + 0.5) * CELL);
                    float wz = (float) (OZ + (iz + 0.5) * CELL);
                    solidVerts.put(wx).put(wy).put(wz);
                    // material color: lighter near top, teal tint
                    float t = (float) iz / (float) NZ;
                    solidColors.put(0.35f + 0.25f * t).put(0.62f).put(0.58f - 0.15f * t);
                    n++;
                }
            }
        }
        solidVerts.position(0);
        solidColors.position(0);
        solidCount = n;

        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, solidVbo);
        GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, n * 3 * 4, solidVerts, GLES30.GL_DYNAMIC_DRAW);
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, solidVboColor);
        GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, n * 3 * 4, solidColors, GLES30.GL_DYNAMIC_DRAW);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES30.glViewport(0, 0, width, height);
        float aspect = (float) width / (float) height;
        Matrix.perspectiveM(proj, 0, 45f, aspect, 1f, 1000f);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT | GLES30.GL_DEPTH_BUFFER_BIT);

        float eyeX = (float) (dist * Math.cos(Math.toRadians(pitch)) * Math.sin(Math.toRadians(yaw)));
        float eyeY = (float) (dist * Math.sin(Math.toRadians(pitch)));
        float eyeZ = (float) (dist * Math.cos(Math.toRadians(pitch)) * Math.cos(Math.toRadians(yaw)));
        Matrix.setLookAtM(view, 0, eyeX, eyeY + 20f, eyeZ, 0, 0, -8f, 0, 1, 0);
        Matrix.multiplyMM(mvp, 0, proj, 0, view, 0);

        // grid
        GLES30.glUseProgram(lineProgram);
        GLES30.glUniformMatrix4fv(lineMvp, 1, false, mvp, 0);
        GLES30.glUniform4f(lineColor, 0.10f, 0.20f, 0.22f, 1f);
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, gridVbo);
        GLES30.glEnableVertexAttribArray(linePos);
        GLES30.glVertexAttribPointer(linePos, 3, GLES30.GL_FLOAT, false, 0, 0);
        GLES30.glDrawArrays(GLES30.GL_LINES, 0, gridCount);

        // frame
        GLES30.glUniform4f(lineColor, 0.0f, 0.55f, 0.50f, 1f);
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, frameVbo);
        GLES30.glVertexAttribPointer(linePos, 3, GLES30.GL_FLOAT, false, 0, 0);
        GLES30.glDrawArrays(GLES30.GL_LINES, 0, frameCount);

        // stock voxels as points
        if (solidCount > 0) {
            GLES30.glUseProgram(solidProgram);
            GLES30.glUniformMatrix4fv(solidMvp, 1, false, mvp, 0);
            GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, solidVbo);
            GLES30.glEnableVertexAttribArray(solidPos);
            GLES30.glVertexAttribPointer(solidPos, 3, GLES30.GL_FLOAT, false, 0, 0);
            GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, solidVboColor);
            GLES30.glEnableVertexAttribArray(solidColor);
            GLES30.glVertexAttribPointer(solidColor, 3, GLES30.GL_FLOAT, false, 0, 0);
            GLES30.glDrawArrays(GLES30.GL_POINTS, 0, solidCount);
        }

        // tool as a small cross
        GLES30.glUseProgram(lineProgram);
        GLES30.glUniformMatrix4fv(lineMvp, 1, false, mvp, 0);
        GLES30.glUniform4f(lineColor, 1f, 0.35f, 0.25f, 1f);
        float tx = (float) toolX, ty = (float) toolY, tz = (float) toolZ;
        float[] tv = {
            tx,ty,tz, tx,ty,tz + 8f,
            tx - 2,ty,tz, tx + 2,ty,tz,
            tx,ty - 2,tz, tx,ty + 2,tz
        };
        FloatBuffer tb = toBuffer(tv);
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, ensureLineVbo());
        GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, tv.length * 4, tb, GLES30.GL_DYNAMIC_DRAW);
        GLES30.glEnableVertexAttribArray(linePos);
        GLES30.glVertexAttribPointer(linePos, 3, GLES30.GL_FLOAT, false, 0, 0);
        GLES30.glDrawArrays(GLES30.GL_LINES, 0, tv.length / 3);
    }

    private int ensureLineVbo() {
        if (lineVbo == 0) {
            int[] b = new int[1];
            GLES30.glGenBuffers(1, b, 0);
            lineVbo = b[0];
        }
        return lineVbo;
    }
}
