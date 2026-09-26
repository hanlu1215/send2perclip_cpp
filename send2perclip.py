"""按 Ctrl+V 依次把剪贴板里的多行内容粘到目标程序里。

关于“偶发跳行”的说明（重要）：
    keyboard.add_hotkey 用的是 Windows 低级键盘钩子，它在按键事件**送到目标程序之前**
    就被调用了。旧的写法一检测到 Ctrl+V 就立刻把下一行写进剪贴板，而目标程序（Excel、
    浏览器输入框等）是在它自己的消息循环里处理按键后才去读剪贴板的。当目标程序响应稍慢
    （窗口重绘、被遮挡、输入法、网页控件等）时，它读到的已经是**下一行**了 ——
    于是第 N 行被跳过、显示成第 N+1 行；而脚本这时才开始等第 N+1 行的粘贴，
    目标程序这次读得快，就把第 N+1 行又粘了一遍。这就是“跳过第 5 个、然后又出现第 6 个”
    的原因，也是它“时好时坏”的原因。

    修复办法：检测到 Ctrl+V 后，先等用户松开按键，再额外等一小段时间（让目标程序把剪贴板
    读完），才写入下一行。如果目标程序很慢，把 PASTE_SETTLE_DELAY 调大即可。
"""

import csv
import threading
import time
from io import StringIO

import keyboard
import pyperclip

# ---------------- 可调参数 ----------------
# 检测到 Ctrl+V 后，等用户松开按键，再额外等待这么久（秒）才写入下一条。
# 目标程序（Excel、网页、输入法）比较慢时，可改成 0.4 甚至 0.5。
PASTE_SETTLE_DELAY = 0.25
# 长按 Ctrl+V 的自动重复 / 极短时间内的重复触发，在这个窗口内只算一次粘贴
DEBOUNCE = 0.15
# 主循环轮询间隔（秒）
POLL_INTERVAL = 0.02
# 每粘贴多少条响一次提示音
BEEP_EVERY = 9


def read_clipboard_rows():
    """读取剪贴板，按 CSV/TSV 解析成一行行数据（每行是单元格列表），并去掉空白行。"""
    content = pyperclip.paste() or ''
    reader = csv.reader(StringIO(content))
    return [row for row in reader if any(cell.strip() for cell in row)]


class PasteWatcher:
    """监听 Ctrl+V / Esc。

    用「计数器」而不是「布尔标志」记录 Ctrl+V：布尔标志在 continue_loop = True
    重置的那一刻如果正好来了 Ctrl+V，这次按键就被吞掉了；计数器不会丢事件。
    """

    def __init__(self):
        self._lock = threading.Lock()
        self._count = 0
        self._last_time = 0.0
        self._exit = False

    # ---- 热键回调 ----
    def on_paste(self):
        now = time.monotonic()
        with self._lock:
            if now - self._last_time < DEBOUNCE:
                return  # 长按产生的自动重复，忽略
            self._last_time = now
            self._count += 1

    def on_exit(self):
        with self._lock:
            self._exit = True

    # ---- 状态读取 ----
    @property
    def count(self):
        with self._lock:
            return self._count

    @property
    def exited(self):
        with self._lock:
            return self._exit

    def wait_for_next(self, target):
        """阻塞等待 Ctrl+V 次数达到 target；返回 False 表示用户按了 Esc。"""
        while True:
            if self.exited:
                return False
            if self.count >= target:
                return True
            time.sleep(POLL_INTERVAL)

    @staticmethod
    def wait_key_release():
        """等用户松开 Ctrl / V，避免长按的自动重复被算成新的粘贴。"""
        while keyboard.is_pressed('ctrl') or keyboard.is_pressed('v'):
            time.sleep(0.01)


def safe_copy(text, retries=10, delay=0.05):
    """目标程序正在占用剪贴板时 pyperclip 会抛异常，这里重试几次。"""
    for _ in range(retries):
        try:
            pyperclip.copy(text)
            return True
        except Exception:
            time.sleep(delay)
    return False


def main():
    data = read_clipboard_rows()
    if not data:
        print('剪贴板里没有可用数据：请先复制要依次粘贴的内容，再运行本程序。')
        return

    watcher = PasteWatcher()
    keyboard.add_hotkey('ctrl+v', watcher.on_paste)
    keyboard.add_hotkey('esc', watcher.on_exit)

    print(f'共 {len(data)} 行数据。按 Ctrl+V 依次粘贴，按 Esc 退出。')
    try:
        # 遍历每一行数据并模拟粘贴
        for index, row in enumerate(data, start=1):
            done_count = watcher.count  # 本次粘贴前的按键计数
            row_data = ', '.join(row)

            if not safe_copy(row_data):
                print(f'[!] 无法写入剪贴板，已跳过: {row_data}')
                continue
            print(f'[{index}/{len(data)}] Copied: {row_data}')

            # 等待用户按下 Ctrl+V（次数要比上一次多 1）或 Esc
            if not watcher.wait_for_next(done_count + 1):
                break

            # ★关键修复：先等按键松开，再给目标程序留出读取剪贴板的时间，
            #   否则目标程序可能在下一条写入后才读到剪贴板 —— 表现为“跳过一行”。
            watcher.wait_key_release()
            time.sleep(PASTE_SETTLE_DELAY)

            # 每粘贴 BEEP_EVERY 条，发出警报音一次
            if index % BEEP_EVERY == 0:
                print('\a')
    finally:
        keyboard.unhook_all_hotkeys()

    print('Script finished.')


if __name__ == '__main__':
    main()
