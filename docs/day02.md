# 動作確認

```bash
# イメージをビルドしてQEMUで実行
make run-nographic

# イメージを build/mnt にマウント
make mount-image

# 書き出されたmemmapファイルを確認
cat build/mnt/memmap

# イメージをアンマウント
make umount-image
```


# memmapの結果


```
Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute
0, 3, EfiBootServicesCode, 00000000, 1, F
1, 7, EfiConventionalMemory, 00001000, 9F, F
2, 7, EfiConventionalMemory, 00100000, 700, F
3, A, EfiACPIMemoryNVS, 00800000, 8, F
4, 7, EfiConventionalMemory, 00808000, 3, F
5, A, EfiACPIMemoryNVS, 0080B000, 1, F
6, 7, EfiConventionalMemory, 0080C000, 4, F
7, A, EfiACPIMemoryNVS, 00810000, F0, F
8, 4, EfiBootServicesData, 00900000, E80, F
9, 7, EfiConventionalMemory, 01780000, 3A3F5, F
10, 4, EfiBootServicesData, 3BB75000, 20, F
11, 7, EfiConventionalMemory, 3BB95000, 231D, F
12, 1, EfiLoaderCode, 3DEB2000, 2, F
13, 7, EfiConventionalMemory, 3DEB4000, 1, F
14, 4, EfiBootServicesData, 3DEB5000, 6D9, F
15, 3, EfiBootServicesCode, 3E58E000, B4, F
16, 4, EfiBootServicesData, 3E642000, 30, F
17, 3, EfiBootServicesCode, 3E672000, E1, F
18, 4, EfiBootServicesData, 3E753000, 65, F
19, 3, EfiBootServicesCode, 3E7B8000, 9, F
20, 4, EfiBootServicesData, 3E7C1000, 5, F
21, 3, EfiBootServicesCode, 3E7C6000, 20, F
22, 4, EfiBootServicesData, 3E7E6000, 2, F
23, 3, EfiBootServicesCode, 3E7E8000, A, F
24, 4, EfiBootServicesData, 3E7F2000, 1, F
25, 3, EfiBootServicesCode, 3E7F3000, 8, F
26, 4, EfiBootServicesData, 3E7FB000, 1, F
27, 3, EfiBootServicesCode, 3E7FC000, 10, F
28, 4, EfiBootServicesData, 3E80C000, 3, F
29, 3, EfiBootServicesCode, 3E80F000, 3, F
30, 4, EfiBootServicesData, 3E812000, 7, F
31, 3, EfiBootServicesCode, 3E819000, 12, F
32, 4, EfiBootServicesData, 3E82B000, 9, F
33, 3, EfiBootServicesCode, 3E834000, E, F
34, 4, EfiBootServicesData, 3E842000, F, F
35, 3, EfiBootServicesCode, 3E851000, B, F
36, 4, EfiBootServicesData, 3E85C000, B, F
37, 3, EfiBootServicesCode, 3E867000, 16, F
38, 4, EfiBootServicesData, 3E87D000, 9, F
39, 3, EfiBootServicesCode, 3E886000, 24, F
40, 4, EfiBootServicesData, 3E8AA000, 3, F
41, 3, EfiBootServicesCode, 3E8AD000, 14, F
42, 4, EfiBootServicesData, 3E8C1000, 7, F
43, 3, EfiBootServicesCode, 3E8C8000, 19, F
44, 4, EfiBootServicesData, 3E8E1000, 3, F
45, 3, EfiBootServicesCode, 3E8E4000, 6, F
46, 4, EfiBootServicesData, 3E8EA000, 2, F
47, 3, EfiBootServicesCode, 3E8EC000, 13, F
48, 4, EfiBootServicesData, 3E8FF000, 5, F
49, 3, EfiBootServicesCode, 3E904000, 16, F
50, 4, EfiBootServicesData, 3E91A000, 2, F
51, 3, EfiBootServicesCode, 3E91C000, D, F
52, 4, EfiBootServicesData, 3E929000, 3, F
53, 3, EfiBootServicesCode, 3E92C000, 6, F
54, 4, EfiBootServicesData, 3E932000, 2, F
55, 3, EfiBootServicesCode, 3E934000, 1, F
56, 4, EfiBootServicesData, 3E935000, 1, F
57, 3, EfiBootServicesCode, 3E936000, 5, F
58, 4, EfiBootServicesData, 3E93B000, 5, F
59, 3, EfiBootServicesCode, 3E940000, C, F
60, 4, EfiBootServicesData, 3E94C000, 2, F
61, 3, EfiBootServicesCode, 3E94E000, 1D, F
62, 4, EfiBootServicesData, 3E96B000, 1, F
63, 3, EfiBootServicesCode, 3E96C000, A, F
64, 4, EfiBootServicesData, 3E976000, 1, F
65, 3, EfiBootServicesCode, 3E977000, 1, F
66, 4, EfiBootServicesData, 3E978000, 2, F
67, 3, EfiBootServicesCode, 3E97A000, 15, F
68, 4, EfiBootServicesData, 3E98F000, 3, F
69, 3, EfiBootServicesCode, 3E992000, 2, F
70, 4, EfiBootServicesData, 3E994000, 2, F
71, 3, EfiBootServicesCode, 3E996000, 7, F
72, 4, EfiBootServicesData, 3E99D000, 7, F
73, 3, EfiBootServicesCode, 3E9A4000, 4, F
74, 4, EfiBootServicesData, 3E9A8000, 6, F
75, 3, EfiBootServicesCode, 3E9AE000, 4, F
76, 4, EfiBootServicesData, 3E9B2000, 2, F
77, 3, EfiBootServicesCode, 3E9B4000, 3B, F
78, 4, EfiBootServicesData, 3E9EF000, 2, F
79, 3, EfiBootServicesCode, 3E9F1000, 4, F
80, 4, EfiBootServicesData, 3E9F5000, 8, F
81, 3, EfiBootServicesCode, 3E9FD000, 3, F
82, 4, EfiBootServicesData, 3EA00000, 202, F
83, 3, EfiBootServicesCode, 3EC02000, 8, F
84, 4, EfiBootServicesData, 3EC0A000, 5, F
85, 6, EfiRuntimeServicesData, 3EC0F000, C1, F
86, 3, EfiBootServicesCode, 3ECD0000, 15, F
87, 4, EfiBootServicesData, 3ECE5000, 1, F
88, 3, EfiBootServicesCode, 3ECE6000, 4, F
89, 4, EfiBootServicesData, 3ECEA000, 3, F
90, 3, EfiBootServicesCode, 3ECED000, 9, F
91, 4, EfiBootServicesData, 3ECF6000, 1, F
92, 3, EfiBootServicesCode, 3ECF7000, 1, F
93, 4, EfiBootServicesData, 3ECF8000, 2, F
94, 3, EfiBootServicesCode, 3ECFA000, 3, F
95, 4, EfiBootServicesData, 3ECFD000, 1, F
96, 3, EfiBootServicesCode, 3ECFE000, 1, F
97, 4, EfiBootServicesData, 3ECFF000, 1, F
98, 3, EfiBootServicesCode, 3ED00000, 16, F
99, 4, EfiBootServicesData, 3ED16000, 1, F
100, 3, EfiBootServicesCode, 3ED17000, 2, F
101, 4, EfiBootServicesData, 3ED19000, 13, F
102, 3, EfiBootServicesCode, 3ED2C000, 2, F
103, 4, EfiBootServicesData, 3ED2E000, 400, F
104, 3, EfiBootServicesCode, 3F12E000, 4, F
105, 4, EfiBootServicesData, 3F132000, 1, F
106, 3, EfiBootServicesCode, 3F133000, 4, F
107, 4, EfiBootServicesData, 3F137000, 6, F
108, 3, EfiBootServicesCode, 3F13D000, A, F
109, 4, EfiBootServicesData, 3F147000, 1, F
110, 3, EfiBootServicesCode, 3F148000, 5, F
111, 4, EfiBootServicesData, 3F14D000, 3A0, F
112, 6, EfiRuntimeServicesData, 3F4ED000, 100, F
113, 5, EfiRuntimeServicesCode, 3F5ED000, 100, F
114, 0, EfiReservedMemoryType, 3F6ED000, 80, F
115, 9, EfiACPIReclaimMemory, 3F76D000, 12, F
116, A, EfiACPIMemoryNVS, 3F77F000, 80, F
117, 4, EfiBootServicesData, 3F7FF000, 601, F
118, 7, EfiConventionalMemory, 3FE00000, 77, F
119, 4, EfiBootServicesData, 3FE77000, 20, F
120, 3, EfiBootServicesCode, 3FE97000, 33, F
121, 4, EfiBootServicesData, 3FECA000, 11, F
122, 3, EfiBootServicesCode, 3FEDB000, 19, F
123, 6, EfiRuntimeServicesData, 3FEF4000, 84, F
124, A, EfiACPIMemoryNVS, 3FF78000, 88, F
125, B, EfiMemoryMappedIO, FFC00000, 400, 1
126, 0, EfiReservedMemoryType, FD00000000, 300000, 0
```