// Qt 事件机制骨架（教学用简化版 C++14 代码，非 Qt 源码）
// 目标：把
// QCoreApplication/QEventLoop/QAbstractEventDispatcher/postEvent/sendEvent/
//      事件队列/定时器/线程 这些概念串起来，方便你把“框架图”落实到代码层。
// 说明：
//  1) 这是“类 Qt”骨架，只保留关键路径，便于阅读。
//  2) 单文件可编译（C++14），仅依赖标准库；用注释标出与 Qt 对应的概念。
//  3) Windows/Unix 的系统消息泵这里用“模拟系统源（SystemSource）”代替。
//  4) 展示：postEvent -> 队列 -> 抽取分发；sendEvent -> 直接分发；
//           QEventLoop::exec 嵌套；processEvents；基本定时器；每线程事件循环。
//  5) 建议：把本文件拆读：底层结构 -> 事件循环 -> 分发路径 -> demo main。

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ===================== 事件与对象 ===================== //
// 对应 Qt::QEvent::Type
enum class EventType {
  None,
  Quit,           // 对应 QEvent::Quit
  User,           // 用户自定义
  Timer,          // 对应 QTimer 触发（Qt: QTimerEvent）
  SystemMessage,  // 模拟来自系统的“原生事件”
};

struct Event {
  EventType type{EventType::None};
  int timerId{-1};            // Timer 用
  void* sysPayload{nullptr};  // 原生事件负载
};

// 简化版 QObject：只保留 event() 与事件过滤器概念
class Object {
 public:
  using Ptr = std::shared_ptr<Object>;

  virtual ~Object() = default;

  // 事件处理入口：对应 QObject::event(QEvent*)
  virtual bool event(Event& e) {
    // 缺省不处理
    return false;
  }

  // 事件过滤器（安装在目标对象上，优先于目标处理）
  using EventFilter =
      std::function<bool(Object*, Event&)>;  // 返回 true 表示吃掉事件

  void installEventFilter(const EventFilter& f) { filters_.push_back(f); }

  bool filterEvent(Event& e) {
    for (auto& f : filters_)
      if (f(this, e)) return true;
    return false;
  }

 private:
  std::vector<EventFilter> filters_;
};

// ===================== 定时器管理（每线程） ===================== //
// 极简定时器：单次/重复都可以用 intervalMs + nextFire 实现
struct TimerInfo {
  int id;                                          // timer id（唯一）
  int intervalMs;                                  // 间隔
  std::chrono::steady_clock::time_point nextFire;  // 下次触发时间
  Object* target;                                  // 回调目标
};

class TimerManager {  // 对应 Qt: QAbstractEventDispatcher 内的定时器设施
 public:
  int startTimer(Object* target, int intervalMs) {
    std::lock_guard<std::mutex> lk(m_);
    int id = ++lastId_;
    TimerInfo t{id, intervalMs,
                std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(intervalMs),
                target};
    timers_[id] = t;
    cv_.notify_all();
    return id;
  }

  void killTimer(int id) {
    std::lock_guard<std::mutex> lk(m_);
    timers_.erase(id);
    cv_.notify_all();
  }

  // 计算距离最近一次触发的等待时间；若需要，生成 Timer 事件
  // 返回：等待毫秒（用于 poll），以及是否生成了事件
  std::pair<int, bool> pumpTimers(std::function<void(Object*, Event&&)> post) {
    using namespace std::chrono;
    std::unique_lock<std::mutex> lk(m_);
    if (timers_.empty()) return {/*wait*/ 50, false};  // 没定时器时，短睡一下

    auto now = steady_clock::now();
    int waitMs = 50;  // 默认
    bool fired = false;

    for (auto& kv : timers_) {
      auto& t = kv.second;
      if (t.nextFire <= now) {
        // 触发一个 Timer 事件（对应 Qt: postEvent 触发 QTimerEvent）
        Event e;
        e.type = EventType::Timer;
        e.timerId = t.id;
        post(t.target, std::move(e));
        t.nextFire = now + milliseconds(t.intervalMs);  // 简化：恒定周期
        fired = true;
      }
      auto diff = duration_cast<milliseconds>(t.nextFire - now).count();
      if (diff < waitMs) waitMs = (int)std::max<int64_t>(0, diff);
    }
    return {waitMs, fired};
  }

  void waitForNext(int waitMs) {
    std::unique_lock<std::mutex> lk(m_);
    cv_.wait_for(lk, std::chrono::milliseconds(waitMs));
  }

 private:
  std::mutex m_;
  std::condition_variable cv_;
  std::unordered_map<int, TimerInfo> timers_;
  int lastId_ = 0;
};

// ===================== 事件派发器 & 事件队列（每线程） =====================
// // 派发器抽象：对应 Qt::QAbstractEventDispatcher
class EventDispatcher {
 public:
  using Clock = std::chrono::steady_clock;

  // 线程局部单例（每线程一个派发器）
  static EventDispatcher& instance() {
    thread_local EventDispatcher d;
    return d;
  }

  // post：把事件加入队列（对应 QCoreApplication::postEvent）
  void post(Object* receiver, Event&& e) {
    {
      std::lock_guard<std::mutex> lk(m_);
      queue_.push({receiver, std::move(e)});
    }
    cv_.notify_all();
  }

  // send：立即分发（对应 QCoreApplication::sendEvent）
  bool send(Object* receiver, Event& e) {
    // 事件过滤器优先
    if (receiver->filterEvent(e)) return true;  // 被吃掉
    return receiver->event(e);
  }

  // 从队列取出并分发（对应：process posted events）
  bool processPostedEvents(int maxCount = 256) {
    int c = 0;
    bool processed = false;
    for (; c < maxCount; ++c) {
      Node n;
      {
        std::lock_guard<std::mutex> lk(m_);
        if (queue_.empty()) break;
        n = std::move(queue_.front());
        queue_.pop();
      }
      processed = true;
      send(n.receiver, n.e);  // 统一走 send 路径，以便触发过滤器/事件
    }
    return processed;
  }

  // 等待：当无事可做时阻塞（简化版）
  void wait(int ms) {
    std::unique_lock<std::mutex> lk(m_);
    if (queue_.empty()) cv_.wait_for(lk, std::chrono::milliseconds(ms));
  }

  TimerManager& timers() { return timers_; }

 private:
  struct Node {
    Object* receiver;
    Event e;
  };

  std::mutex m_;
  std::condition_variable cv_;
  std::queue<Node> queue_;
  TimerManager timers_;
};

// ===================== 模拟系统事件源 ===================== //
// 在真实 Qt 里，Windows 用 MsgWaitForMultipleObjects/PeekMessage，
// X11/Wayland/macOS 用各自 native 循环，这里只做一个“偶尔产生活动”的源。
class SystemSource : public Object {
 public:
  using Callback = std::function<void(const std::string&)>;
  explicit SystemSource(Callback cb) : cb_(std::move(cb)) {}

  void poke(const std::string& msg) {
    Event e;
    e.type = EventType::SystemMessage;
    e.sysPayload = (void*)(&msg_);
    msg_ = msg;  // 存到成员保证生命周期
    EventDispatcher::instance().post(this, std::move(e));
  }

  bool event(Event& e) override {
    if (e.type == EventType::SystemMessage) {
      cb_(msg_);
      return true;
    }
    return false;
  }

 private:
  std::string msg_;
  Callback cb_;
};

// ===================== 事件循环（可嵌套） ===================== //
// 对应 Qt::QEventLoop
class EventLoop {
 public:
  enum ProcessFlag { AllEvents = 0x1, PostedOnly = 0x2 };

  int exec() {               // 返回代码类似 QDialog::exec 的 Accepted/Rejected
    if (running_) return 0;  // 简化：防止重入
    running_ = true;
    exit_ = false;
    rc_ = 0;

    auto& disp = EventDispatcher::instance();
    while (!exit_) {
      // 1) 先处理已投递事件（posted events）
      bool did = disp.processPostedEvents();

      // 2) 处理定时器：计算等待时间并触发到队列
      auto timerRes = disp.timers().pumpTimers(
          [&](Object* r, Event&& e) { disp.post(r, std::move(e)); });

      // 3) 若没有立即可处理的，等待一小会（被 post/计时器唤醒）
      if (!did && !exit_) {
        // 等待系统/队列/定时器
        disp.wait(timerRes.first);
        // （真实 Qt 下，native 源也会被驱动，这里简化）
      }
    }
    running_ = false;
    return rc_;
  }

  void exit(int rc = 0) {
    exit_ = true;
    rc_ = rc;
  }

  // 对应 QEventLoop::processEvents（注意：这里极简实现）
  bool processEvents(int flags = AllEvents) {
    auto& disp = EventDispatcher::instance();
    bool did = disp.processPostedEvents();
    if (flags & AllEvents) {
      auto t = disp.timers().pumpTimers(
          [&](Object* r, Event&& e) { disp.post(r, std::move(e)); });
      (void)t;  // 简化：不主动 wait
    }
    return did;
  }

 private:
  bool running_{false};
  std::atomic<bool> exit_{false};
  int rc_{0};
};

// ===================== 应用程序（每进程） ===================== //
// 对应 Qt::QCoreApplication / QApplication::exec
class Application : public Object {
 public:
  Application() = default;

  int exec() {
    // 主事件循环
    return mainLoop_.exec();
  }

  void quit() { mainLoop_.exit(0); }

  // API 透传
  static void postEvent(Object* r, Event&& e) {
    EventDispatcher::instance().post(r, std::move(e));
  }
  static bool sendEvent(Object* r, Event& e) {
    return EventDispatcher::instance().send(r, e);
  }
  static bool processEvents() { return EventLoop().processEvents(); }

 private:
  EventLoop mainLoop_;
};

// ===================== 一个“控件”示例（演示事件路径） ===================== //
class Widget : public Object {
 public:
  explicit Widget(std::string name) : name_(std::move(name)) {}

  // 模拟点击（系统消息转换为 User 事件）
  void simulateClick() {
    Event e;
    e.type = EventType::User;  // 相当于 QMouseEvent 等
    Application::postEvent(this, std::move(e));
  }

  // 安装一个 1s 的定时器
  void startBlink() {
    timerId_ = EventDispatcher::instance().timers().startTimer(this, 1000);
  }

  bool event(Event& e) override {
    if (e.type == EventType::User) {
      std::cout << name_ << ": received User event (click)\n";
      // 演示嵌套事件循环（比如：对话框/拖拽等会 exec 一个内循环）
      EventLoop nested;
      // 用一个系统源在 500ms 后 poke，届时调用 nested.exit()
      std::thread([this, &nested] {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << name_ << ": nested task done, exiting nested loop\n";
        nested.exit(1);
      }).detach();
      std::cout << name_ << ": entering nested loop...\n";
      int rc = nested.exec();
      std::cout << name_ << ": nested loop exited with rc=" << rc << "\n";
      return true;
    } else if (e.type == EventType::Timer && e.timerId == timerId_) {
      std::cout << name_ << ": timer tick (id=" << timerId_ << ")\n";
      tick_++;
      if (tick_ >= 3) {
        std::cout << name_ << ": stop timer after 3 ticks\n";
        EventDispatcher::instance().timers().killTimer(timerId_);
      }
      return true;
    }
    return false;
  }

 private:
  std::string name_;
  int timerId_{-1};
  int tick_{0};
};

// ===================== Demo ===================== //
int main() {
  Application app;  // 对应 QApplication a(argc, argv);

  Widget w("MainWidget");

  // 事件过滤器示例：优先于对象处理
  w.installEventFilter([](Object* self, Event& e) {
    (void)self;
    if (e.type == EventType::User) {
      std::cout << "[Filter] consume one User event\n";
      // 返回 true 表示吃掉：这次不会进入 Widget::event
      // 为了演示，我们只吃掉第一次，其它放行
      static bool first = true;
      if (first) {
        first = false;
        return true;
      }
    }
    return false;
  });

  // 系统源：把“原生事件”转成框架事件
  SystemSource sys(
      [](const std::string& s) { std::cout << "[System] " << s << "\n"; });

  // 模拟：启动定时器 + 多次点击 + 系统消息
  w.startBlink();
  w.simulateClick();  // 第一次将被过滤器吃掉
  w.simulateClick();  // 第二次进入 Widget::event，并在其中跑嵌套循环

  std::thread([&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    sys.poke("native message #1");
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    sys.poke("native message #2");
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    // 触发退出应用主循环（对应 QCoreApplication::quit）
    Event e;
    e.type = EventType::Quit;
    Application::postEvent(&app, std::move(e));
  }).detach();

  // Application 也能处理事件：比如 Quit
  struct AppHandler : Application {
    bool event(Event& e) override {
      if (e.type == EventType::Quit) {
        std::cout << "[App] Quit received, exiting main loop...\n";
        quit();
        return true;
      }
      return false;
    }
  };

  // 运行主循环
  std::cout << "== entering main loop ==\n";
  int rc = app.exec();
  std::cout << "== main loop exited rc=" << rc << " ==\n";
  return 0;
}

/*
阅读路径建议：
1) 事件从哪来？
   - 用户代码：Widget::simulateClick -> Application::postEvent ->
EventDispatcher::post -> 队列
   - 定时器：TimerManager::pumpTimers 每轮把到时的定时器转换为 Timer 事件并 post
到队列
   - 系统源：SystemSource::poke -> post SystemMessage 到队列（真实 Qt
由原生消息泵提供）

2) 事件如何被分发？
   - EventLoop::exec 循环里反复调用 EventDispatcher::processPostedEvents，
     每次从队列取出一个 Node，先过 filterEvent，再调用 receiver->event。
   - sendEvent 则是同步直达（不入队、不切换循环），可导致“重入”。

3) 嵌套循环如何发生？
   - 在对象处理某事件时创建另一个 EventLoop 并 exec()，
     外层循环未退出，但当前栈帧阻塞在内层循环，直到内层 exit()。
     典型场景：QDialog::exec、拖拽、同步等待子操作完成等。

4) processEvents 有何作用？
   - 在计算密集/阻塞过程中，手动调用 EventLoop::processEvents 以便“喘口气”，
     处理已入队的事件与定时器，避免 UI 卡死（注意可导致重入）。

5) 与 Qt 的映射：
   - Application            ~ QCoreApplication/QApplication
   - EventLoop              ~ QEventLoop
   - EventDispatcher        ~ QAbstractEventDispatcher (+ 平台具体实现)
   - TimerManager           ~ QTimer/QAbstractEventDispatcher 的定时器机制
   - SystemSource           ~ 平台原生消息源 (WinMsg/XCB/CFRunLoop...)
   - Object::event/filter  ~ QObject::event / installEventFilter
   - postEvent/sendEvent    ~ QCoreApplication::postEvent / sendEvent
*/
