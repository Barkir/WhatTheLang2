# WhatTheLang (basically just python copy)

## Preamble
- [What is used](#used)
- [To do](#to-do)

## Used
- [My Assembler](https://github.com/Barkir/Compiler)

## Syntax and how to use
```
git clone https://github.com/Barkir/WhatTheLang2
```
- Language has math operations (+, -, >, < etc.), conditioins (if, while).
- Also you can create functions with param (code example in toRun folder)
- Create variables
  ```
  a = 20;
  b = 30;
  ```
  - Use conditions or while
    ```
    if (a == 20)
    {
      print(a + 30);
      a = 700;
    }

    while (b < 20)
    {
      print(b);
      b = b - 1;
    }
    ```


- This is how you can create functions
```
def function(a, b, c)
{
// your code here
}
```

## To do
- [] Add error processing


## Source
Spasibo dedu za Huawei
efefeefe




# IM BACK B%%CHES


# Binary Translator
- Now let's turn our language into real binary translator


Let's see what we have now.

| Language Components |
|---------------------|
| 1 level -> language parser |
| 2 level -> AST-tree representation |
| 3 level -> AST 2 IR representation |
| 4 level -> IR to own binary representation|

My Intermediate represenation is connected with binary representation.
But what if i want to run the binary file on any PC?
GCC compiler creates **elf** binary file out of our code.
Our compiler should do the same.

## First chapter (writing libraries)
Starting with our own **IO library** written in **NASM64**.
We need to write to calls - **input** and **output**
They're similar to the ones we use in [purintf](https://github.com/Barkir/Purintf). Although WhatTheLang uses float numbers, let's write all the calls for *integers* at first. It is just easier and faster at this stage.


## Second chapter (rewriting our previous code)

...

## Third chapter (ELF-file structure)
elf-file consists of:
1. Header
2. Data

To read elf-file we can use ```readelf```

First for bytes are defining our file as elf-file

```7f 45 4c 46```

Next byte is defining class field:
|Value |Architecture |
|-------|------------|
01 | 32-bit architecture |
02 | 64-bit architecture |

Next byte is defining data field:

|Value |Data setting |
|-------|------------|
01 | LSB (Little Endian) |
02 | MSB (Big Endian) |

Next byte is for version number and it is always ```01```

## OPCODES

#### MOV

In my translation I use 3 types of mov's
1. mov reg, number
2. mov reg, reg
3. mov [reg], reg
4. mov reg, [reg]

| Operation | Opcode |
|-----------|--------|
| ```mov reg, val``` | ```{B8+reg_code}{val}```


##### REGISTER SEQUENCE CODE TABLE
| Register | Byte-code | Instruction |
|----------|-----------|-------------|
| eax | 000 | ```b8```
| ecx | 001 |```b9```
| edx | 010 |```ba```
| ebx | 011 |```bb```
| esp | 100 |```bc```
| ebp | 101 |```bd```
| esi | 110 |```be```
| edi | 111 |```bf```

When we use mov in value-mode with r11, r12, r13, r14, r15 we add 41 as prefix

| Register | Byte-code | Instruction |
|----------|-----------|-------------|
| r8 | 000 | ```41b8```
| r9 | 001 |```41b9```
| r10 | 010 |```41ba```
| r11 | 011 |```41bb```
| r12 | 100 |```41bc```
| r13 | 101 |```41bd```
| r14 | 110 |```41be```
| r15 | 111 |```41bf```


#### PUSH

| Type | Opcode |
|------|--------|
| register | 0x50 |
| imm32 | 0x68 |
| imm8  | 0x6a |

Imm32 push will be used for values. It is a 5-byte operation (1 byte for opcode and 4 bytes for imm32 value)


```
68 00 00 00 00
   ^         ^
   |_________|
    int value
```

Register Push requires to use this formula
```
0x50 + reg_sequence number
```
You can find appropriate reg_seqeunce number [here](#register-sequence-code-table)

If you use extended registers (r8, r9, ..., r15) add special byte **0x41** to the start



#### POP

Pop is available for registers only


#### ADD





