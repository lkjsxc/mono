# arch

## byte_base

|opecode|op2|op1|
|---|---|---|
|2bit|3bit|3bit|

## register

|name|description|
|---|---|
|reg0|calculation_left, input|
|reg1|calculation_right|
|reg2|calculation_result, output|

## opcode

|code|description|
|---|---|
|00|immediate|
|01|calculation|
|10|copy|
|11|system|

## immediate
0 ~ 63 to reg0

## calculation
reg0, reg1 to reg2

|code|description|
|---|---|
|000|or|
|001|nand|
|010|nor|
|011|and|
|100|add|
|101|sub|
|110|mul|
|111|xor|

## system

|code|name|description|
|---|---|---|
|000|input|input to reg0|
|001|output|reg2 to output|
|010|mem_load|addr is reg3, load to reg0|
|011|mem_save|addr is reg3, value is reg2|
|100|jmp|addr is reg3|
|101|jze|addr is reg3, condition is reg2|
|110|jnz|addr is reg3, condition is reg2|