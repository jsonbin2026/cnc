# GK Android App

Android 前端骨架，直接编译仓库中的 C 语言 `gk` 引擎（CNC 教学模拟器核心，1525 项功能目录），
通过 JNI 把目录数据暴露给原生 Android 界面。

## 目录结构

```
android/
├── settings.gradle
├── build.gradle
├── gradle.properties
├── gradlew / gradlew.bat
├── gradle/wrapper/gradle-wrapper.properties
└── app/
    ├── build.gradle
    ├── src/main/AndroidManifest.xml
    ├── src/main/java/com/example/gk/MainActivity.java
    ├── src/main/res/layout/activity_main.xml
    └── src/main/cpp/
        ├── CMakeLists.txt          # 引入 gk 源码 + 编译 native-lib.so
        ├── native-lib.c            # JNI 桥接
        └── gk/                     # 从 /workspace/gk 拷入的 C 引擎源码
            ├── CMakeLists.txt
            ├── include/
            └── src/
```

## 前置条件

- Android Studio（推荐，自带 SDK / NDK / Gradle），或
- 单独安装：JDK 17、Android SDK（含 build-tools、platform 34）、NDK、CMake 3.22.1

## 构建 APK

### 方式一：Android Studio

直接用 Android Studio 打开 `android/` 目录，等待 Gradle 同步后：

- 菜单 `Build > Build Bundle(s) / APK(s) > Build APK(s)`
- 产物：`app/build/outputs/apk/debug/app-debug.apk`

### 方式二：命令行

首次需要生成 Gradle wrapper 的 jar（若本机已装 gradle）：

```bash
cd android
gradle wrapper --gradle-version 8.7
```

然后构建：

```bash
# 调试版 APK
./gradlew assembleDebug

# 发布版 APK（需在 app/build.gradle 配置 signingConfigs）
./gradlew assembleRelease
```

产物路径：

```
app/build/outputs/apk/debug/app-debug.apk
```

## 安装到设备

```bash
# 手机开启 USB 调试后
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## 更新 C 引擎

当 `/workspace/gk` 的源码变更后，重新同步到 Android 工程：

```bash
cp -r /workspace/gk/include /workspace/android/app/src/main/cpp/gk/
cp -r /workspace/gk/src     /workspace/android/app/src/main/cpp/gk/
# 注意：cpp/gk/CMakeLists.txt 已去掉 -Werror，覆盖后请手动恢复该修改
```

## 说明

- 当前界面只是把 1525 项功能目录（id/名称/域/状态）渲染成可滚动列表，用于验证 C 引擎在 Android 上可以正常编译与调用。
- 若要做成完整模拟器 UI，需要在 `native-lib.c` 里追加更多 JNI 入口，并在 Java/Kotlin 层绘制交互。
- 引擎仅依赖标准 C 与 libm，无需额外 NDK 三方库。
