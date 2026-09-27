@echo off
setlocal
set APP_HOME=%~dp0
set CLASSPATH=%APP_HOME%gradle\wrapper\gradle-wrapper.jar
if not exist "%CLASSPATH%" (
  echo Missing %CLASSPATH%
  echo Generate it with: gradle wrapper --gradle-version 8.7
  exit /b 1
)
if defined JAVA_HOME (set JAVACMD=%JAVA_HOME%\bin\java.exe) else (set JAVACMD=java)
"%JAVACMD%" -Xmx2048m -classpath "%CLASSPATH%" org.gradle.wrapper.GradleWrapperMain %*
