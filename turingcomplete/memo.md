# arch

## inst

|opecode|dst|src|
|---|---|---|
|2bit|3bit|3bit|

## opcode

|code|description|
|---|---|
|00|immediate|
|01|calculation|
|10|copy|
|11|system|

## calculation

|code|description|
|---|---|
|000|or|
|001|nand|
|010|nor|
|011|and|
|100|add|
|101|sub|
|110|mul|

## register

|name|description|
|---|---|
|reg0|calculation_left, input|
|reg1|calculation_right|
|reg2|calculation_result, output|

## system

|code|description|
|---|---|
|000|input|
|001|output|
|010|mem_load|
|011|mem_save|
|100|jmp|
|101|jze|
|110|jnz|