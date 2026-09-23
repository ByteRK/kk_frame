### 字库生成

1. 在out目录下编译字库生成程序
~~~
make dictbuild
~~~

2. 将输出的字库生成程序拷贝到目标机器上

3. 拷贝`./data`到目标机器上，注意该文件夹需与字库生成程序在同一目录下。
   数据源以 gzip 存放（约 1MB，解压后约 3.4MB），需先解压：
~~~
gunzip -k ./data/*.gz
~~~
   解压后应得到 `data/rawdict_utf16_65105_freq.txt` 与 `data/valid_utf16.txt`
   （生成程序按此文件名读取，不可改名）

4. 在目标机器上运行字库生成程序，即可产生对应架构的字库文件
~~~
./dictbuild
~~~

> ⚠️ **字库是「架构相关」的**：内部按 `sizeof(size_t)` 写入头部计数，
> 32 位程序生成的 `dict_pinyin.dat` 在 64 位程序下加载会直接 `bad_alloc` 崩溃，
> 反之亦然。**因此字库必须在目标架构上生成**（如 x64 调试用 x64 的
> `dictbuild`，板子用 ARM 版），把 x64 生成的字库拷到板子上同样会崩。
>
> 键盘在打开字库前会做校验（存在性 + 字长 + 计数合理性），不通过时
> 仅打印 `LOGE` 并降级为「无拼音」，不会崩溃。

### 字库使用

1. 将生成的字库文件拷贝到目标机器，文件名保持为
   `dict_pinyin.dat`（系统字库）与 `userdict.dat`（用户字库，可为空文件）

2. 运行主程序，即可加载字库文件（字库路径定义见 `lib_keyboard_config.h.in`，
   相对程序运行目录，即 `pinyin/dict_pinyin.dat`、`pinyin/userdict.dat`）
