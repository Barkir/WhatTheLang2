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






