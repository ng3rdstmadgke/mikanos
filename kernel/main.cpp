#include <cstdint>

// extern "C" はC言語からこの関数呼び出すためマングリングを行わないようにする記述
extern "C" void KernelMain(uint64_t frame_buffer_base, uint64_t frame_buffer_size) {
  // reinterpret_cast:
  //   ある型のビットパターンをそのまま別の型として再解釈するキャスト。C言語の(uint8_t*)frame_buffer_baseと効果は全く同じ
  uint8_t* frame_buffer = reinterpret_cast<uint8_t*>(frame_buffer_base);
  for (uint64_t i = 0; i < frame_buffer_size; i++) {
    frame_buffer[i] = i % 256;
  }
  // __asm__() はインラインアセンブリ。C言語からアセンブリ命令を呼び出すことができる
  // hlt はCPUを停止させる命令で省電力状態になる。割り込みがあると動作が再開する。(永久ループにするとCPUが100%に張り付いてしまう)
  while (1) __asm__("hlt");
}