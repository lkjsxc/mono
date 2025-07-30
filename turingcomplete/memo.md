# arch

## byte_base

|opecode|op2|op1|
|---|---|---|
|2bit|3bit|3bit|

## register

|name|description|
|---|---|
|reg0|calculation_left, io|
|reg1|calculation_right|
|reg2||
|reg3||
|reg4||
|reg5|stack_addr|
|reg6|jmp_addr|
|reg7|ram_addr|

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
reg0, reg1 to reg0

|code|description|
|---|---|
|0000|or|
|0001|and|
|0010|xor|
|0011|not|
|0100|add|
|0101|sub|
|0110|shl|
|0111|shr|
|1000|nop|
|1001|eq|
|1010|neq|
|1011|lt|

## system

|code|name|description|
|---|---|---|
|0000|input|input to reg0|
|0001|output|reg0 to output|
|0010|mem_load|addr is reg7, load to reg0|
|0011|mem_save|addr is reg7, value is reg0|
|0100|jmp|addr is reg6|
|0101|jze|addr is reg6, condition is reg0|
|0110|push|mem_save then inc reg5|
|0111|pop|dec reg5 then mem_load|