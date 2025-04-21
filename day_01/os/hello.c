typedef unsigned short CHAR16;
typedef unsigned long long EFI_STATUS;
typedef void *EFI_HANDLE;

// _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL構造体の宣言。(定義は後で)
// この構造体をポインタ型として先に使いたいため宣言のみ行う
struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_TEXT_STRING)(
  struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  CHAR16                                  *String
);

// _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL 構造体に EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL という別名をつける
typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  void            *dummy;  // void* 型はどんな方のポインタにもなれる汎用ポインタ。サイズ合わせや将来の拡張のために存在する
  EFI_TEXT_STRING OutputString;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

// 構造体を定義して EFI_SYSTEM_TABLE という別名を付ける
typedef struct {
  char dummy[52];
  EFI_HANDLE ConsoleOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
} EFI_SYSTEM_TABLE;

// エントリーポイント
EFI_STATUS EfiMain(
  EFI_HANDLE IageHandle,
  EFI_SYSTEM_TABLE *SystemTable
) {
  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello, world!\n");
  while(1);
  return 0;
}