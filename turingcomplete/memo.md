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
|000|or|
|001|nand|
|010|nor|
|011|and|
|100|add|
|101|sub|
|110|shl|
|111|shr|

## system

|code|name|description|
|---|---|---|
|0000|input|input to reg0|
|0001|output|reg0 to output|
|0010|mem_load|addr is reg7, load to reg0|
|0011|mem_save|addr is reg7, value is reg0|
|0100|jmp|addr is reg6|
|0101|je|addr is reg6, condition is reg0|
|0110|jne|addr is reg6, condition is reg0|
|0111|ja|addr is reg6, condition is reg0|
|1000|push|mem_save then inc reg5|
|1001|pop|dec reg5 then mem_load|