package com.example.gk;

/** GLSL shader sources for the simulator. */
final class Shaders {
    private Shaders() {}

    static final String VERTEX =
        "#version 300 es\n" +
        "uniform mat4 uMvp;\n" +
        "in vec3 aPos;\n" +
        "in vec3 aColor;\n" +
        "out vec3 vColor;\n" +
        "void main() {\n" +
        "    vColor = aColor;\n" +
        "    gl_Position = uMvp * vec4(aPos, 1.0);\n" +
        "}\n";

    static final String FRAGMENT =
        "#version 300 es\n" +
        "precision mediump float;\n" +
        "in vec3 vColor;\n" +
        "out vec4 fragColor;\n" +
        "void main() {\n" +
        "    fragColor = vec4(vColor, 1.0);\n" +
        "}\n";

    static final String LINE_VERTEX =
        "#version 300 es\n" +
        "uniform mat4 uMvp;\n" +
        "in vec3 aPos;\n" +
        "void main() {\n" +
        "    gl_Position = uMvp * vec4(aPos, 1.0);\n" +
        "}\n";

    static final String LINE_FRAGMENT =
        "#version 300 es\n" +
        "precision mediump float;\n" +
        "uniform vec4 uColor;\n" +
        "out vec4 fragColor;\n" +
        "void main() {\n" +
        "    fragColor = uColor;\n" +
        "}\n";
}
