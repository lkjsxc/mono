# global_val

## register
001. = f64 zero
002. = f64 stack pointer
003. = f64 base pointer
004. = f64 global pointer

## tmp
005. = any tmp01
006. = any tmp02
007. = any tmp03
008. = any tmp04
009. = any tmp05
010. = any tmp06
011. = any tmp07
012. = any tmp08

## tmp vec
017. = vec<any> tmp_vec1
018. = vec<any> tmp_vec2
019. = vec<any> tmp_vec3
020. = vec<any> tmp_vec4

## global struct
033. = vec<node> node_data
034. = vec<node*> node_free
035. = vec<obj> obj_data
036. = vec<obj*> obj_free
037. = map<obj*, NULL> obj_all

## clone
049. = vec<obj*> obj_clone

## global tmp
257. = map01
258. = map02
259. = map03
260. = map04

## type
### debug
513. = map<obj*, NULL> type_debug01
514. = map<obj*, NULL> type_debug02

## stack
1025. ~

# obj
00. type
01. x
02. y