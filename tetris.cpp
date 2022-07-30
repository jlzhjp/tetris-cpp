#include <iostream>
#include <random>

#define NOMINMAX
#include <Windows.h>

#define MAP_HEIGHT 20
#define MAP_WIDTH 10

#define PIXEL "  "

#define BACKGROUND_WHITE (BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN)
#define BACKGROUND_YELLOW (BACKGROUND_RED | BACKGROUND_GREEN)
#define BACKGROUND_CYAN (BACKGROUND_BLUE | BACKGROUND_GREEN)
#define BACKGROUND_MAGENTA (BACKGROUND_RED | BACKGROUND_BLUE)
#define BACKGROUND_BLACK 0

#define BACKGROUND_INTENSE_RED (BACKGROUND_RED | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_GREEN (BACKGROUND_GREEN | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_BLUE (BACKGROUND_BLUE | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_WHITE (BACKGROUND_WHITE | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_YELLOW (BACKGROUND_YELLOW | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_CYAN (BACKGROUND_CYAN | BACKGROUND_INTENSITY)
#define BACKGROUND_INTENSE_MAGENTA (BACKGROUND_MAGENTA | BACKGROUND_INTENSITY)

typedef WORD Frame[MAP_HEIGHT][MAP_WIDTH];

struct Block {
  int size;
  WORD color;
  bool *matrix;
};

#define _ false
#define O true

bool matrix_L[3][3] = {
  {O, _, _},
  {O, O, O},
  {_, _, _},
};

bool matrix_L_mirror[3][3] = {
  {_, _, O},
  {O, O, O},
  {_, _, _},
};

bool matrix_I[4][4] = {
  {_, _, O, _},
  {_, _, O, _},
  {_, _, O, _},
  {_, _, O, _},
};

bool matrix_T[3][3] = {
  {_, O, _},
  {O, O, O},
  {_, _, _},
};

bool matrix_O[2][2] = {
  {O, O},
  {O, O},
};

bool matrix_Z[3][3] = {
  {O, O, _},
  {_, O, O},
  {_, _, _},
};

bool matrix_Z_mirror[3][3] = {
  {_, O, O},
  {O, O, _},
  {_, _, _},
};

#undef _
#undef O

Block predefined_blocks[] = {
  { 4, BACKGROUND_CYAN, (bool *)matrix_I },
  { 3, BACKGROUND_MAGENTA, (bool *)matrix_T },
  { 2, BACKGROUND_YELLOW, (bool *)matrix_O },
  { 3, BACKGROUND_BLUE, (bool *)matrix_L },
  { 3, BACKGROUND_INTENSE_YELLOW, (bool *)matrix_L_mirror },
  { 3, BACKGROUND_RED, (bool *)matrix_Z },
  { 3, BACKGROUND_GREEN, (bool *)matrix_Z_mirror },
};


// 全局变量
// --------

// 控制台句柄与属性
HANDLE h_input = GetStdHandle(STD_INPUT_HANDLE);
HANDLE h_output = GetStdHandle(STD_OUTPUT_HANDLE);
WORD default_attributes = 0;

// 游戏显示
Frame current_frame = {};
Frame next_frame = {};
Frame fixed_frame = {};

// 状态记录
Block current_block = {};
COORD current_block_pos = {};

// 临时缓冲区
bool rotation_buffer[64] = {};

// 随机数引擎
std::default_random_engine random_engine((unsigned int)time(0));

// 控制台操控函数
// --------------

// 初始化句柄及参数
void initialize_console();

// 设置控制台输出文本的属性
inline void set_text_attribute(WORD attributes);

// 使用初始化时备份的属性重置控制台文本属性
inline void reset_text_attribute();

// 设置控制台光标的位置
inline void set_cursor_position(short x, short y);

// 阻塞并从控制台读取按键
KEY_EVENT_RECORD read_key();

// 控制台清屏
void clear_console();


// 绘图函数
// --------

// 绘制边框
void draw_border();

// 填充单个像素
void fill_pixel_on_frame(int x, int y, WORD attributes, Frame frame);

// 将当前正在下落的方块绘制到指定帧上
void draw_current_block_to(Frame frame);

// 开始绘制下一帧
void begin_draw_frame();

// 结束绘制，更新当前帧, 并将当前帧绘制到控制台上
void end_draw_frame();

// 绘制当前正在下落的方块
inline void draw_current_block();

// 将当前正在下落的方块固定到固定帧上
inline void fix_current_block();

// 清空游戏区域
void clear_map();


// 游戏控制函数
// ------------

// 初始化游戏
void initialize_game();

// 更新当前正在下落的方块
void new_current_block();

// 将当前正在下落的方块向下移动一格
// 移动成功返回 true, 移动失败(碰到了其他方块)返回 true
bool move_current_block_down();

// 将当前正在正在下落的方块向左移动一格
// 移动成功返回 true, 移动失败(碰到了其他方块)返回 false
bool move_current_block_left();

// 将当前正在正在下落的方块向右移动一格
// 移动成功返回 true, 移动失败(碰到了其他方块)返回 false
bool move_current_block_right();

// 旋转当前正在下落的方块
// 旋转成功返回 true, 旋转失败(碰到了其他方块)返回 false
bool rotate_current_block();

// 检查某一方块是否与与已经固定的其他方块或者边框重叠
// 发生重叠返回 true, 未发生重叠返回 false
bool is_block_overlapped(bool *matrix, int x, int y, int size);

// 检查当前的方块是否与已经固定的其他方块或者边框重叠
// 发生重叠返回 true, 未发生重叠返回 false
inline bool is_current_block_overlapped();

// 清除fixed_frame中已经满的行, 返回清除的行数
int remove_full_row();

// 检查游戏当前的状态
// 游戏失败(固定帧中有砖块突破顶部)返回 false, 否则返回 true
bool check_game_status();


// 游戏周期函数:
// ---------

// 打印开始界面
void print_start_page();

// 开始游戏
int start_game_loop();

// 打印结束界面
void print_over_page(int score);

int main() {
  clear_console();
  initialize_console();
  draw_border();

  while (true) {
    print_start_page();
    read_key();
    clear_map();
    int score = start_game_loop();
    print_over_page(score);
    read_key();
    clear_map();
  }

  return 0;
}

void initialize_game() {
  const size_t size = MAP_WIDTH * MAP_HEIGHT * sizeof(WORD);
  std::memset(fixed_frame, 0, size);
  std::memset(current_frame, 0, size);
  std::memset(next_frame, 0, size);
  std::memset(&current_block, 0, sizeof(Block));
  std::memset(&current_block_pos, 0, sizeof(COORD));
}

void print_start_page() {
  set_cursor_position(2, 8);
  std::cout << "TETRIS";

  set_cursor_position(2, 10);
  std::cout << "向左: ";
  set_text_attribute(FOREGROUND_BLUE);
  std::cout << "<LEFT>";
  reset_text_attribute();

  set_cursor_position(2, 11);
  std::cout << "向右: ";
  set_text_attribute(FOREGROUND_BLUE);
  std::cout << "<RIGHT>";
  reset_text_attribute();

  set_cursor_position(2, 12);
  std::cout << "向下: ";
  set_text_attribute(FOREGROUND_BLUE);
  std::cout << "<DOWN>";
  reset_text_attribute();

  set_cursor_position(2, 13);
  std::cout << "移动到底: ";
  set_text_attribute(FOREGROUND_BLUE);
  std::cout << "<SPACE>";
  reset_text_attribute();

  set_cursor_position(2, 15);
  set_text_attribute(FOREGROUND_RED);
  std::cout << "按任意键开始...";
  reset_text_attribute();
}

void print_over_page(int score) {
  set_cursor_position(2, 8);
  set_text_attribute(FOREGROUND_RED);
  std::cout << "游戏结束";
  reset_text_attribute();

  set_cursor_position(2, 9);
  std::cout << "得分: ";
  set_text_attribute(FOREGROUND_GREEN);
  std::cout << score;
  reset_text_attribute();

  set_cursor_position(2, 15);
  set_text_attribute(FOREGROUND_BLUE);
  std::cout << "按任意键继续...";
  reset_text_attribute();
}

int start_game_loop() {
  initialize_game();

  INPUT_RECORD records[128] = {};
  DWORD len = 0;
  int score = 0;

  new_current_block();
  while (true) {
    Sleep(300);

    bool should_current_block_fixed = false;

    begin_draw_frame();

    PeekConsoleInput(h_input, records, 1, &len);
    if (len != 0) {
      ReadConsoleInput(h_input, records, 128, &len);
    }
    for (DWORD i = 0; i != len; ++i) {
      if (records[i].EventType == KEY_EVENT
        && records[i].Event.KeyEvent.bKeyDown) {

        KEY_EVENT_RECORD record = records[i].Event.KeyEvent;

        switch (record.wVirtualKeyCode) {
        case VK_UP:
          for (int i = 0; i != record.wRepeatCount; ++i) {
            if (!rotate_current_block()) {
              break;
            }
          }
          break;
        case VK_DOWN:
          for (int i = 0; i != record.wRepeatCount; ++i) {
            if (!move_current_block_down()) {
              should_current_block_fixed = true;
              break;
            }
          }
          break;
        case VK_LEFT:
          for (int i = 0; i != record.wRepeatCount; ++i) {
            if (!move_current_block_left()) {
              break;
            }
          }
          break;
        case VK_RIGHT:
          for (int i = 0; i != record.wRepeatCount; ++i) {
            if (!move_current_block_right()) {
              break;
            }
          }
          break;
        case VK_SPACE:
          while (move_current_block_down()) {
            /**/
          }
          should_current_block_fixed = true;
          break;
        }
      }
    }

    if (!move_current_block_down()) {
      should_current_block_fixed = true;
    }

    draw_current_block();

    end_draw_frame();

    if (should_current_block_fixed) {
      if (!check_game_status()) {
        break;
      }
      fix_current_block();
      new_current_block();
      score += remove_full_row();
    }
  }

  return score;
}

void new_current_block() {
  int count = sizeof(predefined_blocks) / sizeof(predefined_blocks[0]);
  std::uniform_int_distribution<unsigned> block_random(0, count - 1);
  current_block = predefined_blocks[block_random(random_engine)];

  std::uniform_int_distribution<short> pos_random(0, MAP_WIDTH - current_block.size);
  current_block_pos = { pos_random(random_engine), (short)-current_block.size };

  std::uniform_int_distribution<short> rotation_random(0, 3);
  short rotation_times = rotation_random(random_engine);
  for (short i = 0; i != rotation_times; ++i) {
    rotate_current_block();
  }
}

bool move_current_block_down() {
  ++current_block_pos.Y;
  if (is_current_block_overlapped()) {
    --current_block_pos.Y;
    return false;
  }
  return true;
}

bool move_current_block_left() {
  --current_block_pos.X;
  if (is_current_block_overlapped()) {
    ++current_block_pos.X;
    return false;
  }
  return true;
}

bool move_current_block_right() {
  ++current_block_pos.X;
  if (is_current_block_overlapped()) {
    --current_block_pos.X;
    return false;
  }
  return true;
}

bool rotate_current_block() {
  const int size = current_block.size;
  const size_t buffer_size = (size_t)size * size;

  for (int i = 0; i != size; ++i) {
    for (int j = 0; j != size; ++j) {
      rotation_buffer[j * size + (size - i - 1)] = current_block.matrix[i * size + j];
    }
  }

  if (is_block_overlapped(rotation_buffer,
    current_block_pos.X, current_block_pos.Y, size)) {
    return false;
  }

  memcpy_s(current_block.matrix, buffer_size, rotation_buffer, buffer_size);
  return true;
}

bool is_block_overlapped(bool *matrix, int x, int y, int size) {
  for (int i = 0; i != size; ++i) {
    if (y + i < 0) {
      continue;
    }

    for (int j = 0; j != size; ++j) {
      if (matrix[i * size + j]
        && (x + j < 0 || x + j >= MAP_WIDTH || y + i >= MAP_HEIGHT
          || fixed_frame[y + i][x + j])) {
        return true;
      }
    }
  }

  return false;
}

bool is_current_block_overlapped() {
  const int x = current_block_pos.X;
  const int y = current_block_pos.Y;
  const int size = current_block.size;
  return is_block_overlapped(current_block.matrix, x, y, size);
}

int remove_full_row() {
  int row_cleared = 0;
  for (int i = 0; i != MAP_HEIGHT; ++i) {
    bool is_current_row_full = true;
    for (int j = 0; j != MAP_WIDTH; ++j) {
      if (!fixed_frame[i][j]) {
        is_current_row_full = false;
        break;
      }
    }

    if (is_current_row_full) {
      ++row_cleared;
      for (int j = i; j != 0; --j) {
        for (int k = 0; k != MAP_WIDTH; ++k) {
          fixed_frame[j][k] = fixed_frame[j - 1][k];
        }
      }
      for (int j = 0; j != MAP_WIDTH; ++j) {
        fixed_frame[0][j] = false;
      }
    }
  }
  return row_cleared;
}

bool check_game_status() {
  const int size = current_block.size;
  for (int i = 0; i != size; ++i) {
    for (int j = 0; j != size; ++j) {
      if (current_block.matrix[i * size + j] && current_block_pos.Y + j < 0) {
        return false;
      }
    }
  }
  return true;
}

void draw_border() {
  set_text_attribute(BACKGROUND_WHITE);
  for (int i = 0; i != MAP_WIDTH + 2; ++i) {
    std::cout << PIXEL;
  }

  for (int i = 1; i <= MAP_HEIGHT; ++i) {
    set_cursor_position(0, i);
    std::cout << PIXEL;
    set_cursor_position((MAP_WIDTH + 1) * 2, i);
    std::cout << PIXEL;
  }

  std::cout << "\n";

  for (int i = 0; i != MAP_WIDTH + 2; ++i) {
    std::cout << PIXEL;
  }
  reset_text_attribute();
}

void begin_draw_frame() {
  for (int i = 0; i != MAP_HEIGHT; ++i) {
    for (int j = 0; j != MAP_WIDTH; ++j) {
      next_frame[i][j] = fixed_frame[i][j];
    }
  }
}

void end_draw_frame() {
  for (int i = 0; i != MAP_HEIGHT; ++i) {
    for (int j = 0; j != MAP_WIDTH; ++j) {
      if (next_frame[i][j] != current_frame[i][j]) {
        set_text_attribute(next_frame[i][j]);
        // 跳过边框的宽度, 所以要加一
        set_cursor_position((j + 1) * 2, i + 1);
        std::cout << PIXEL;
        reset_text_attribute();
        current_frame[i][j] = next_frame[i][j];
      }
    }
  }
  set_cursor_position(0, MAP_HEIGHT + 3);
}

void fill_pixel_on_frame(int x, int y, WORD attributes, Frame frame) {
  if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) {
    return;
  }

  frame[y][x] = attributes;
}

void draw_current_block_to(Frame frame) {
  const int size = current_block.size;
  const COORD pos = current_block_pos;

  for (int i = 0; i != size; ++i) {
    for (int j = 0; j != size; ++j) {
      if (current_block.matrix[i * size + j]) {
        fill_pixel_on_frame(pos.X + j, pos.Y + i, current_block.color, frame);
      }
    }
  }
}

void draw_current_block() {
  draw_current_block_to(next_frame);
}

void fix_current_block() {
  draw_current_block_to(fixed_frame);
}

void clear_map() {
  for (int i = 0; i != MAP_HEIGHT; ++i) {
    DWORD temp;
    short length = MAP_WIDTH * 2;
    COORD start_coord = { 2, short(i + 1) };
    FillConsoleOutputAttribute(h_output, default_attributes, length, start_coord, &temp);
    FillConsoleOutputCharacter(h_output, L' ', length, start_coord, &temp);
  }
}

void set_text_attribute(WORD attributes) {
  SetConsoleTextAttribute(h_output, attributes);
}

void reset_text_attribute() {
  set_text_attribute(default_attributes);
}

void initialize_console() {
  CONSOLE_SCREEN_BUFFER_INFO info;
  GetConsoleScreenBufferInfo(h_output, &info);
  default_attributes = info.wAttributes;
}

void set_cursor_position(short x, short y) {
  SetConsoleCursorPosition(h_output, { x, y });
}

KEY_EVENT_RECORD read_key() {
  INPUT_RECORD record;
  DWORD record_num;
  for (;;) {
    if (!ReadConsoleInput(h_input, &record, 1, &record_num) || record_num == 0) {
      std::cerr << "无法获取键盘输入\n";
      exit(1);
    }

    if (record.EventType == KEY_EVENT) {
      KEY_EVENT_RECORD key_event = record.Event.KeyEvent;
      if (key_event.bKeyDown) {
        return key_event;
      }
    }
  }
}

void clear_console() {
  CONSOLE_SCREEN_BUFFER_INFO info;
  GetConsoleScreenBufferInfo(h_output, &info);
  DWORD length = info.dwSize.Y * info.dwSize.X;
  DWORD written;
  COORD position = { 0, 0 };
  FillConsoleOutputAttribute(h_output, default_attributes, length, position, &written);
  FillConsoleOutputCharacterW(h_output, L' ', length, position, &written);
  SetConsoleCursorPosition(h_output, position);
}