# Qt 事件机制骨架（简化模拟）

**目的**：以单文件代码演示 Qt 事件循环/派发/定时器/嵌套循环/事件过滤器的基本工作流。

## 构建与运行

### 使用 CMake（Windows / Linux / macOS）

```bash
cd examples/qt_eventloop_skeleton

cmake -S . -B build

cmake --build build --config Release

./build/qt_eventloop_skeleton   # Windows 下为 build/Release/qt_eventloop_skeleton.exe
```

### 使用 MSVC（VS2022）

- 打开 VS -> Open -> CMake... 选择本目录；

- 或者新建一个空的 Console App，把 `main.cpp` 加进去即可。

## 说明

- 这是教学用“类 Qt”骨架，便于映射到 Qt：

- `Application` ~ `QCoreApplication/QApplication`

- `EventLoop` ~ `QEventLoop`

- `EventDispatcher` ~ `QAbstractEventDispatcher`

- 定时器/事件过滤器/posted vs send 等均有示例。

- 没有依赖 Qt 库，完全离线可编译运行。